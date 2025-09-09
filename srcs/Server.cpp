#include "Server.hpp"
#include "Utils.hpp"
#include "Client.hpp" // Include Client.hpp here
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

    // Set non-blocking mode
    if (Utils::setnonblocking(sock_fd) < 0) {
        close(sock_fd);
        throw std::runtime_error("Failed to set non-blocking mode on server socket");
    }
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

void Server::serverRun() {
    // To be implemented after refining Client and other utilities
}

void Server::sigHandler(int sig)
{
    (void)sig;
    Logger::warning("Signal Received! Stopping server...");
    signal = true;
}
