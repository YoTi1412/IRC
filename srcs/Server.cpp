#include "Server.hpp"
#include "Utils.hpp"
#include "Client.hpp"
#include "Logger.hpp"
#include <cerrno>

bool Server::signal = false;

Server::Server(const std::string &portStr, const std::string &password)
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
    if (sock_fd > 0) close(sock_fd);
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
}

void Server::setSocketTimeout() {
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        close(sock_fd);
        throw std::runtime_error("Failed to set socket timeout: " + std::string(strerror(errno)));
    }
}

void Server::bindSocket() {
    if (bind(sock_fd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        close(sock_fd);
        throw std::runtime_error("Failed to bind socket: " + std::string(strerror(errno)));
    }
}

void Server::listenOnSocket() {
    if (listen(sock_fd, SOMAXCONN) < 0) {
        close(sock_fd);
        throw std::runtime_error("Failed to listen on socket: " + std::string(strerror(errno)));
    }
}

void Server::initPollFd() {
    pollFds.clear();
    pollFds.push_back(pollfd());
    pollFds.back().fd = sock_fd;
    pollFds.back().events = POLLIN;
    pollFds.back().revents = 0;

    // Set non-blocking mode and reserve space for multiple clients
    if (Utils::setnonblocking(sock_fd) < 0) {
        close(sock_fd);
        throw std::runtime_error("Failed to set non-blocking mode on server socket");
    }
    pollFds.reserve(100); // Reserve space for up to 100 clients to avoid reallocation
}

void Server::serverInit() {
    signal = false;

    createSocket();
    configureServerAddress();
    setSocketTimeout();
    bindSocket();
    listenOnSocket();
    initPollFd();

    Logger::info("Server initialized on port " + Utils::intToString(port));
}

void Server::pollForEvents() {
    if (poll(&pollFds[0], pollFds.size(), -1) < 0) {
        if (errno == EINTR) return;
        Logger::error("Poll failed: " + std::string(strerror(errno)));
    }
}

void Server::acceptNewConnection() {
    struct sockaddr_in clientAddress;
    socklen_t clientLen = sizeof(clientAddress);
    int client_fd = accept(sock_fd, (struct sockaddr*)&clientAddress, &clientLen);
    if (client_fd < 0) {
        Logger::warning("Failed to accept connection: " + std::string(strerror(errno)));
        return;
    }

    if (Utils::setnonblocking(client_fd) < 0) {
        close(client_fd);
        Logger::warning("Failed to set non-blocking mode on client socket");
        return;
    }

    Client* newClient = new Client();
    newClient->setFd(client_fd);
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(clientAddress.sin_addr), ipStr, INET_ADDRSTRLEN);
    newClient->setIPAddress(ipStr);
    clients[client_fd] = newClient;

    pollFds.push_back(pollfd());
    pollFds.back().fd = client_fd;
    pollFds.back().events = POLLIN;
    pollFds.back().revents = 0;

    Logger::info("New client connected, fd: " + Utils::intToString(client_fd) + ", IP: " + ipStr);
}

void Server::handleClientData(std::vector<pollfd>::iterator& it) {
    char buffer[1024] = {0};
    int bytesRead = read(it->fd, buffer, sizeof(buffer) - 1);
    if (bytesRead < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            Logger::warning("Read error on fd: " + Utils::intToString(it->fd) + ", " + std::string(strerror(errno)));
        }
        return;
    } else if (bytesRead == 0) {
        cleanupDisconnectedClient(it);
    } else {
        buffer[bytesRead] = '\0';
        std::string command(buffer);
        if (clients.find(it->fd) != clients.end()) {
            clients[it->fd]->processCommand(command);
        }
    }
}

void Server::cleanupDisconnectedClient(std::vector<pollfd>::iterator& it) {
    Logger::info("Client disconnected, fd: " + Utils::intToString(it->fd));
    close(it->fd);
    delete clients[it->fd];
    clients.erase(it->fd);
    it = pollFds.erase(it);
    --it; // Adjust iterator after erase
}

void Server::serverRun() {
    while (!signal) {
        pollForEvents();
        for (std::vector<pollfd>::iterator it = pollFds.begin(); it != pollFds.end(); ++it) {
            if (it->revents & POLLIN) {
                if (it->fd == sock_fd) {
                    acceptNewConnection();
                } else {
                    handleClientData(it);
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
