#include "Server.hpp"
#include "Utils.hpp"
#include "Client.hpp"
#include "Logger.hpp"

bool Server::signal = false;

Server::Server(const std::string &portStr, const std::string &password)
    : processedFds()
{
    validateArgs(portStr, password);
    name = "ircserv";
    port = std::atoi(portStr.c_str());
    this->password = password;
    createdtime = Utils::getFormattedTime();
    Logger::info("Server instance created with port " + portStr + " and password set.");
}

void Server::validateArgs(const std::string &portStr, const std::string &password) {
    checkEmptyArgs(portStr, password);
    validatePort(portStr);
    validatePassword(password);
}

void Server::checkEmptyArgs(const std::string &portStr, const std::string &password) {
    if (portStr.empty() || password.empty()) {
        Logger::error("Validation failed: Arguments are empty!");
        throw std::invalid_argument("Arguments are empty!");
    }
}

void Server::validatePort(const std::string &portStr) {
    if (!Utils::isValidPort(portStr.c_str())) {
        Logger::error("Validation failed: Invalid port number. Must be between 1024 and 65535.");
        throw std::invalid_argument("Invalid port number. Must be between 1024 and 65535.");
    }
}

void Server::validatePassword(const std::string &password) {
    if (!Utils::isValidPassword(password)) {
        Logger::error("Validation failed: Invalid password. No spaces or non-printable characters allowed.");
        throw std::invalid_argument("Invalid password. No spaces or non-printable characters allowed.");
    }
}

Server::~Server() {
    cleanupAllClients();
    closeSocket();
    logShutdown();
}

void Server::cleanupAllClients() {
    std::map<int, Client*>::iterator it = clients.begin();
    while (it != clients.end()) {
        close(it->first);
        delete it->second;
        ++it;
    }
    clients.clear();
    Logger::info("All clients cleaned up.");
}

void Server::closeSocket() {
    if (sock_fd > 0) {
        close(sock_fd);
        Logger::info("Server socket closed.");
    }
}

void Server::logShutdown() {
    Logger::info("Server on port " + Utils::intToString(port) + " is shutting down.");
}

// ------------------- Socket Initialization -------------------

void Server::serverInit() {
    createSocket();
    configureServerAddress();
    setSocketTimeout();
    bindSocket();
    listenOnSocket();
    initPollFd();
    logInitialization();
}

void Server::createSocket() {
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        Logger::error("Failed to create socket: " + std::string(strerror(errno)));
        throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
    }
    Logger::info("Socket created successfully.");
}

void Server::configureServerAddress() {
    std::memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);
    Logger::debug("Server address configured with port " + Utils::intToString(port));
}

void Server::setSocketTimeout() {
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) < 0) {
        Logger::error("Failed to set socket timeout: " + std::string(strerror(errno)));
        throw std::runtime_error("Failed to set socket timeout: " + std::string(strerror(errno)));
    }
    Logger::debug("Socket timeout set to 5 seconds.");
}

void Server::bindSocket() {
    if (bind(sock_fd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        Logger::error("Failed to bind socket: " + std::string(strerror(errno)));
        throw std::runtime_error("Failed to bind socket: " + std::string(strerror(errno)));
    }
    Logger::info("Socket bound successfully.");
}

void Server::listenOnSocket() {
    if (listen(sock_fd, SOMAXCONN) < 0) {
        Logger::error("Failed to listen on socket: " + std::string(strerror(errno)));
        throw std::runtime_error("Failed to listen on socket: " + std::string(strerror(errno)));
    }
    Logger::info("Server listening on socket with maximum connections.");
}

void Server::initPollFd() {
    pollfd pfd;
    pfd.fd = sock_fd;
    pfd.events = POLLIN;
    pollFds.push_back(pfd);
    Logger::debug("Poll file descriptor initialized for socket.");
}

void Server::logInitialization() {
    Logger::info("Server initialized on port " + Utils::intToString(port));
}

// ------------------- Event Loop -------------------

void Server::serverRun() {
    while (!signal) {
        std::vector<pollfd> pollCopy = pollFds;
        pollForEvents(pollCopy);
        processedFds.clear();
        processEvents(pollCopy);
    }
    Logger::info("Server run loop terminated due to signal.");
}

void Server::pollForEvents(std::vector<pollfd>& pollCopy) {
    if (poll(&pollCopy[0], pollCopy.size(), -1) < 0) {
        Logger::error("Poll failed: " + std::string(strerror(errno)));
        throw std::runtime_error("Poll failed: " + std::string(strerror(errno)));
    }
    Logger::debug("Poll event checked for " + Utils::intToString(pollCopy.size()) + " file descriptors.");
}

void Server::processEvents(const std::vector<pollfd>& pollCopy) {
    unsigned int i = 0;
    while (i < pollCopy.size()) {
        if (pollCopy[i].revents & POLLIN) {
            if (pollCopy[i].fd == sock_fd) {
                acceptNewConnection();
            } else {
                handleClientPoll(pollCopy[i].fd);
            }
        }
        ++i;
    }
    Logger::debug("Processed events for " + Utils::intToString(pollCopy.size()) + " file descriptors.");
}

void Server::handleClientPoll(int fd) {
    std::vector<pollfd>::iterator it = pollFds.begin();
    while (it != pollFds.end()) {
        if (it->fd == fd) {
            handleClientData(it);
            break;
        }
        ++it;
    }
    Logger::debug("Handled poll event for client fd: " + Utils::intToString(fd));
}

// ------------------- Client Connection -------------------

void Server::acceptNewConnection() {
    sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    int clientFd = accept(sock_fd, (struct sockaddr*)&clientAddr, &clientLen);
    handleAcceptResult(clientFd, clientAddr);
}

void Server::handleAcceptResult(int clientFd, sockaddr_in& clientAddr) {
    if (clientFd < 0) {
        Logger::warning("Accept failed: " + std::string(strerror(errno)));
        return;
    }
    configureNewClient(clientFd, clientAddr);
}

void Server::configureNewClient(int clientFd, sockaddr_in& clientAddr) {
    Utils::setnonblocking(clientFd);
    pollfd pfd;
    pfd.fd = clientFd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    pollFds.push_back(pfd);
    Client* client = createNewClient(clientFd, clientAddr);
    clients[clientFd] = client;
}

Client* Server::createNewClient(int clientFd, sockaddr_in& clientAddr) {
    Client* client = new Client();
    client->setFd(clientFd);
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(clientAddr.sin_addr), ip, INET_ADDRSTRLEN);
    client->setIPAddress(ip);
    logNewConnection(clientFd, ip);
    return client;
}

void Server::logNewConnection(int clientFd, const char* ip) {
    Logger::info("New client connected, fd: " + Utils::intToString(clientFd) + ", IP: " + ip);
}

// ------------------- Client Data Handling -------------------

void Server::handleClientData(std::vector<pollfd>::iterator& it) {
    char buffer[BUFFER_SIZE] = {0};
    int bytesRead = read(it->fd, buffer, sizeof(buffer) - 1);
    processReadResult(it->fd, buffer, bytesRead);
}

void Server::processReadResult(int fd, char* buffer, int bytesRead) {
    if (bytesRead < 0) {
        handleReadError(fd);
    } else if (bytesRead == 0) {
        handleClientDisconnect(fd);
    } else {
        handleReadSuccess(fd, buffer, bytesRead);
    }
}

void Server::handleReadError(int fd) {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
        Logger::warning("Read error on fd: " + Utils::intToString(fd) + ", " + strerror(errno));
    }
}

void Server::handleClientDisconnect(int fd) {
    std::vector<pollfd>::iterator it = findPollIterator(fd);
    if (it != pollFds.end() && processedFds.find(fd) == processedFds.end()) {
        logDisconnect(fd);
        close(it->fd);
        delete clients[fd];
        clients.erase(fd);
        it = pollFds.erase(it);
        processedFds.insert(fd);
    }
}

std::vector<pollfd>::iterator Server::findPollIterator(int fd) {
    std::vector<pollfd>::iterator it = pollFds.begin();
    while (it != pollFds.end()) {
        if (it->fd == fd) {
            return it;
        }
        ++it;
    }
    return pollFds.end();
}

void Server::logDisconnect(int fd) {
    Logger::info("Client disconnected, fd: " + Utils::intToString(fd));
}

void Server::handleReadSuccess(int fd, char* buffer, int bytesRead) {
    buffer[bytesRead] = '\0';
    Logger::debug("Read " + Utils::intToString(bytesRead) + " bytes from fd " + Utils::intToString(fd) + ": " + buffer);
    appendToClientBuffer(fd, buffer);
    processClientBuffer(fd);
}

void Server::appendToClientBuffer(int fd, const char* data) {
    if (clients.find(fd) != clients.end()) {
        clients[fd]->getCommandBuffer() += data;
    }
}

void Server::processClientBuffer(int fd) {
    Client* client = clients[fd];
    std::string& buffer = client->getCommandBuffer();
    while (hasNextMessage(buffer)) {
        std::string message = extractNextMessage(buffer);
        std::list<std::string> cmdList = parseMessage(message);
        if (!cmdList.empty()) {
            executeCommand(fd, cmdList);
        }
    }
}

bool Server::hasNextMessage(const std::string& buffer) {
    if (buffer.find('\n') != std::string::npos) {
        return true;
    }
    return false;
}

std::string Server::extractNextMessage(std::string& buffer) {
    size_t pos = buffer.find('\n');
    std::string message = extractMessage(buffer, pos);
    buffer.erase(0, pos + 1);
    return message;
}

std::string Server::extractMessage(const std::string& buffer, size_t pos) {
    bool hasCR = false;
    if (pos > 0 && buffer[pos - 1] == '\r') {
        hasCR = true;
    }
    size_t end;
    if (hasCR) {
        end = pos - 1;
    } else {
        end = pos;
    }
    return buffer.substr(0, end);
}

std::list<std::string> Server::parseMessage(const std::string& message) {
    std::list<std::string> cmdList;
    size_t colonPos = message.find(" :");
    std::string prefix = message;
    std::string trailing;

    if (colonPos != std::string::npos) {
        prefix = message.substr(0, colonPos);
        trailing = message.substr(colonPos + 2);
    }

    tokenizePrefix(prefix, cmdList);
    if (!trailing.empty()) {
        cmdList.push_back(trailing);
    }

    return cmdList;
}

void Server::tokenizePrefix(const std::string& prefix, std::list<std::string>& cmdList) {
    std::istringstream iss(prefix);
    std::string token;
    while (iss >> token) {
        cmdList.push_back(token);
    }
}

// ------------------- Command Execution -------------------

void Server::executeCommand(int fd, std::list<std::string> cmdList) {
    if (cmdList.empty()) {
        return;
    }

    std::string cmd = cmdList.front();
    if (!isUpperCase(cmd)) {
        sendInvalidCommandError(fd, cmd);
        return;
    }

    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
    Client* client = clients[fd];
    dispatchCommand(cmd, cmdList, client);
}

void Server::sendInvalidCommandError(int fd, const std::string& cmd) {
    clients[fd]->sendReply(":ircserv " ERR_NEEDMOREPARAMS " * " + cmd + " :Commands must be uppercase\r\n");
    Logger::warning("Invalid command received from fd " + Utils::intToString(fd) + ": " + cmd);
}

void Server::dispatchCommand(const std::string& cmd, std::list<std::string> cmdList, Client* client) {
    if (cmd == "PASS") {
        handlePass(cmdList, client, this);
    } else if (cmd == "NICK") {
        handleNick(cmdList, client);
    } else if (cmd == "USER") {
        handleUser(cmdList, client, this);
    } else if (cmd == "JOIN") {
        handleJoin(cmdList, client);
    } else {
        sendUnknownCommandError(client, cmd);
    }
}

void Server::sendUnknownCommandError(Client* client, const std::string& cmd) {
    client->sendReply(":ircserv " ERR_UNKNOWNCOMMAND " " + (client->getNickname().empty() ? "*" : client->getNickname()) + " " + cmd + " :Unknown command\r\n");
    Logger::warning("Unknown command received from client " + client->getNickname() + ": " + cmd);
}

bool Server::isUpperCase(const std::string& str) {
    unsigned int i = 0;
    while (i < str.size()) {
        if (!std::isupper(str[i])) {
            return false;
        }
        ++i;
    }
    return true;
}

// ------------------- Signal Handling -------------------

void Server::sigHandler(int sig) {
    (void)sig;
    Logger::warning("Signal received! Stopping server...");
    signal = true;
}
