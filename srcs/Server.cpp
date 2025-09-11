#include "Server.hpp"
#include "Utils.hpp"
#include "Client.hpp"
#include "Logger.hpp"

bool Server::signal = false;

Server::Server(const std::string &portStr, const std::string &password)
    : processedFds()
{
    if (portStr.empty() || password.empty()) {
        throw std::invalid_argument("Arguments are empty!");
    }
    if (!Utils::isValidPort(portStr.c_str())) {
        throw std::invalid_argument("Invalid port number. Must be between 1024 and 65535.");
    }
    if (!Utils::isValidPassword(password)) {
        throw std::invalid_argument("Invalid password. No spaces or non-printable characters allowed.");
    }
    name = "ircserv";
    this->port = std::atoi(portStr.c_str());
    this->password = password;
    createdtime = Utils::getFormattedTime();
}

Server::~Server()
{
    for (std::map<int, Client*>::iterator it = clients.begin(); it != clients.end(); ++it) {
        close(it->first);
        delete it->second;
    }
    clients.clear();
    if (sock_fd > 0)
        close(sock_fd);
    Logger::info("Server with port " + Utils::intToString(port) + " is shutting down.");
}

Server::Server(const Server &server) {
    *this = server;
}

Server &Server::operator=(const Server &server)
{
    if (this != &server) {
        name = server.name;
        port = server.port;
        password = server.password;
        createdtime = server.createdtime;
    }
    return (*this);
}

const std::string &Server::getName() const
{
    return name;
}

const std::string &Server::getCreatedTime() const
{
    return createdtime;
}

void Server::createSocket() {
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
    }
}

void Server::configureServerAddress() {
    std::memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);
    // RFC 2812 Reference: Section 2.1 - Server address configuration
}

void Server::setSocketTimeout() {
    struct timeval tv;
    tv.tv_sec = 5; // 5-second timeout
    tv.tv_usec = 0;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) < 0) {
        throw std::runtime_error("Failed to set socket timeout: " + std::string(strerror(errno)));
    }
}

void Server::bindSocket() {
    if (bind(sock_fd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        throw std::runtime_error("Failed to bind socket: " + std::string(strerror(errno)));
    }
}

void Server::listenOnSocket() {
    if (listen(sock_fd, SOMAXCONN) < 0) {
        throw std::runtime_error("Failed to listen on socket: " + std::string(strerror(errno)));
    }
}

void Server::initPollFd() {
    pollfd pfd;
    pfd.fd = sock_fd;
    pfd.events = POLLIN;
    pollFds.push_back(pfd);
}

void Server::pollForEvents(std::vector<pollfd>& pollCopy) {
    if (poll(&pollCopy[0], pollCopy.size(), -1) < 0) {
        throw std::runtime_error("Poll failed: " + std::string(strerror(errno)));
    }
}

void Server::acceptNewConnection() {
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    int clientFd = accept(sock_fd, (struct sockaddr*)&clientAddr, &clientLen);
    if (clientFd < 0) {
        Logger::warning("Accept failed: " + std::string(strerror(errno)));
        return;
    }
    Utils::setnonblocking(clientFd);
    pollfd pfd;
    pfd.fd = clientFd;
    pfd.events = POLLIN;
    pollFds.push_back(pfd);
    Client* client = new Client();
    client->setFd(clientFd);
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(clientAddr.sin_addr), ip, INET_ADDRSTRLEN);
    client->setIPAddress(ip);
    clients[clientFd] = client;
    Logger::info("New client connected, fd: " + Utils::intToString(clientFd) + ", IP: " + ip);
    // RFC 2812 Reference: Section 3.1 - Client connection handling
}

void Server::handleClientData(std::vector<pollfd>::iterator& it) {
    char tempBuffer[BUFFER_SIZE] = {0};
    int bytesRead = read(it->fd, tempBuffer, sizeof(tempBuffer) - 1);
    if (bytesRead < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            Logger::warning("Read error on fd: " + Utils::intToString(it->fd) + ", " + std::string(strerror(errno)));
        }
        return;
    } else if (bytesRead == 0) {
        if (processedFds.find(it->fd) == processedFds.end()) {
            cleanupDisconnectedClient(it); // Fixed: Matches updated declaration
            processedFds.insert(it->fd);
        }
    } else {
        tempBuffer[bytesRead] = '\0';
        Logger::debug("Read " + Utils::intToString(bytesRead) + " bytes from fd " + Utils::intToString(it->fd) + ": " + std::string(tempBuffer));
        if (clients.find(it->fd) != clients.end()) {
            Client* client = clients[it->fd];
            client->getCommandBuffer() += std::string(tempBuffer);

            std::string& buffer = client->getCommandBuffer();
            while (true) {
                size_t pos = buffer.find('\n');
                if (pos == std::string::npos) break;

                bool has_cr = (pos > 0 && buffer[pos - 1] == '\r');
                size_t msg_end = has_cr ? pos - 1 : pos;
                size_t term_size = has_cr ? 2 : 1;

                std::string message = buffer.substr(0, msg_end);

                // Parse message into cmdList
                std::list<std::string> cmdList;
                size_t colon_pos = message.find(" :");
                std::string prefix;
                std::string trailing;
                if (colon_pos != std::string::npos) {
                    prefix = message.substr(0, colon_pos);
                    trailing = message.substr(colon_pos + 2);
                } else {
                    prefix = message;
                }

                std::istringstream iss(prefix);
                std::string token;
                while (iss >> token) {
                    cmdList.push_back(token);
                }

                if (!trailing.empty()) {
                    cmdList.push_back(trailing);
                }

                // Execute if cmdList not empty
                if (!cmdList.empty()) {
                    executeCommand(it->fd, cmdList);
                }

                // Erase processed part
                buffer.erase(0, msg_end + term_size);
            }
        }
    }
}

void Server::executeCommand(int fd, std::list<std::string> cmdList) {
    if (cmdList.empty()) return;
    std::string cmd = *cmdList.begin();
    // Check if command is not uppercase
    for (size_t i = 0; i < cmd.length(); ++i) {
        if (!std::isupper(cmd[i])) {
            Client* client = clients[fd];
            client->sendReply(":ircserv 461 * " + cmd + " :Commands must be uppercase\r\n"); // ERR_NEEDMOREPARAMS with custom message
            return;
        }
    }
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper); // Ensure internal consistency
    Client* client = clients[fd];
    Server* server = this;

    if (cmd == "PASS") {
        handlePass(cmdList, client, this);
    } else if (cmd == "NICK") {
        handleNick(cmdList, client);
    } else if (cmd == "USER") {
        handleUser(cmdList, client, server);
    } else if (cmd == "JOIN") {
        handleJoin(cmdList, client);
    } else {
        client->sendReply(":ircserv 421 " + (client->getNickname().empty() ? "*" : client->getNickname()) + " " + cmd + " :Unknown command\r\n");
    }
    size_t pos = client->getCommandBuffer().find(CRLF);
    if (pos != std::string::npos) {
        client->getCommandBuffer().erase(0, pos + 2);
    }
}

void Server::cleanupDisconnectedClient(std::vector<pollfd>::iterator& it) {
    Logger::info("Client disconnected, fd: " + Utils::intToString(it->fd));
    close(it->fd);
    delete clients[it->fd];
    clients.erase(it->fd);
    it = pollFds.erase(it);
    --it;
}

void Server::serverRun() {
    while (!signal) {
        std::vector<pollfd> pollCopy = pollFds;
        pollForEvents(pollCopy);
        processedFds.clear();
        for (size_t i = 0; i < pollCopy.size(); ++i) {
            if (pollCopy[i].revents & POLLIN) {
                if (pollCopy[i].fd == sock_fd) {
                    acceptNewConnection();
                } else {
                    for (std::vector<pollfd>::iterator it = pollFds.begin(); it != pollFds.end(); ++it) {
                        if (it->fd == pollCopy[i].fd) {
                            handleClientData(it);
                            break;
                        }
                    }
                }
            }
        }
    }
}

void Server::sigHandler(int sig)
{
    (void)sig;
    Logger::warning("Signal Received! Stopping server...");
    signal = true;
}

void Server::serverInit() {
    createSocket();
    configureServerAddress();
    setSocketTimeout();
    bindSocket();
    listenOnSocket();
    initPollFd();
    Logger::info("Server initialized on port " + Utils::intToString(port));
}

const std::string& Server::getPassword() const { return password; }
