#include "Client.hpp"
#include "Utils.hpp"
#include "Logger.hpp"
#include "Server.hpp" // For hypothetical command dispatch (to be implemented)

Client::Client() : fd(-1), registered(false), authenticated(false), nickSet(false), userSet(false) {}
Client::~Client() {
    if (fd >= 0) close(fd);
    Logger::info("Client disconnected, fd: " + Utils::intToString(fd));
}

int Client::getFd() const { return fd; }
std::string Client::getIPAddress() const { return IPAddress; }
std::string Client::getNickname() const { return nickname; }
std::string Client::getUsername() const { return username; }
std::string Client::getHostname() const { return hostname; }

bool Client::isRegistered() const { return registered; }
bool Client::isAuthenticated() const { return authenticated; }
bool Client::isNickSet() const { return nickSet; }
bool Client::isUserSet() const { return userSet; }

void Client::setAuthenticated() { authenticated = true; }
void Client::setRegistered() { registered = true; }
void Client::setFd(int fd) { this->fd = fd; }
void Client::setIPAddress(const std::string& IPAddress) { this->IPAddress = IPAddress; }
void Client::setNickname(const std::string& nickname) { this->nickname = nickname; }
void Client::setHostname(const std::string& hostname) { this->hostname = hostname; }
void Client::setUsername(const std::string& username) { this->username = username; }
void Client::setRegistered(bool status) { this->registered = status; }
void Client::setNickSet(bool nickSet) { this->nickSet = nickSet; }
void Client::setUserSet(bool userSet) { this->userSet = userSet; }

void Client::processCommand(const std::string& command) {
    getCommandBuffer() += command; // Append new data
    // Removed placeholder command processing as it's now handled in Server
    // Clear buffer after CRLF (optional fallback)
    size_t pos = getCommandBuffer().find("\n");
    if (pos != std::string::npos) {
        getCommandBuffer().erase(0, pos + 1);
    }
}

void Client::sendReply(const std::string& reply) {
    std::string formattedReply = reply;
    if (formattedReply.rfind(CRLF) != formattedReply.length() - 2) {
        formattedReply += CRLF;
    }
    if (formattedReply.length() > 512) {
        Logger::warning("Reply too long for fd " + Utils::intToString(fd) + ", truncating to 510 bytes + CRLF");
        formattedReply = formattedReply.substr(0, 510) + CRLF;
    }
    ssize_t bytesSent = send(fd, formattedReply.c_str(), formattedReply.length(), 0);
    if (bytesSent < 0) {
        Logger::error("Failed to send reply to fd " + Utils::intToString(fd) + ": " + std::string(strerror(errno)));
    } else if (static_cast<size_t>(bytesSent) < formattedReply.length()) {
        Logger::warning("Partial send to fd " + Utils::intToString(fd) + ": " + Utils::intToString(bytesSent) + "/" + Utils::intToString(formattedReply.length()) + " bytes");
    } else {
        Logger::debug("Sent " + Utils::intToString(bytesSent) + " bytes to fd " + Utils::intToString(fd) + ": " + reply);
    }
    // RFC 2812 Reference: Section 2.3 - Messages must end with CRLF and not exceed 512 bytes
}

std::string& Client::getCommandBuffer() {
    return commandBuffer;
}
