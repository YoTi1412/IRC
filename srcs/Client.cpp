#include "Client.hpp"
#include "Utils.hpp"
#include "Logger.hpp"

Client::Client() : fd(-1), registered(false) {}
Client::~Client() {
    if (fd >= 0) close(fd);
    Logger::info("Client disconnected, fd: " + Utils::intToString(fd));
}

int Client::getFd() const { return fd; }
std::string Client::getIPAddress() const { return IPAddress; }
std::string Client::getNickname() const { return nickname; }
std::string Client::getUsername() const { return username; }
bool Client::isRegistered() const { return registered; }

void Client::setFd(int fd) { this->fd = fd; }
void Client::setIPAddress(const std::string& IPAddress) { this->IPAddress = IPAddress; }
void Client::setNickname(const std::string& nickname) { this->nickname = nickname; }
void Client::setUsername(const std::string& username) { this->username = username; }
void Client::setRegistered(bool status) { this->registered = status; }

void Client::processCommand(const std::string& command) {
    // Placeholder for IRC command processing (e.g., PASS, NICK, USER)
    Logger::debug("Received command from fd " + Utils::intToString(fd) + ": " + command);
    // Implement parsing and command handling here later
}
