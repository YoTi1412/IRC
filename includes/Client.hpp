#pragma once

#include "Includes.hpp"

#define MAX_MESSAGE_LENGTH 512
#define MAX_MESSAGE_BODY 510

#define LOG_CLIENT_CREATED "Client instance created."
#define LOG_CLIENT_DISCONNECTED(fd) ("Client disconnected, fd: " + Utils::intToString(fd))
#define LOG_NICK_SET(nick) ("Nickname set to: " + nick)
#define LOG_USERNAME_SET(user) ("Username set to: " + user)
#define LOG_HOSTNAME_SET(host) ("Hostname set to: " + host)
inline std::string LOG_AUTH_STATUS(bool status) {
    if (status) {
        return "Client authentication status set to: true";
    } else {
        return "Client authentication status set to: false";
    }
}
inline std::string LOG_REG_STATUS(bool status) {
    if (status) {
        return "Client registration status set to: true";
    } else {
        return "Client registration status set to: false";
    }
}
#define LOG_PARTIAL_SEND(fd, sent, total) ("Partial send to fd " + Utils::intToString(fd) + ": " + Utils::intToString(sent) + "/" + Utils::intToString(total) + " bytes")
#define LOG_SEND_FAILED(fd, err) ("Failed to send reply to fd " + Utils::intToString(fd) + ": " + err)
#define LOG_SEND_TRUNCATED(fd) ("Reply too long for fd " + Utils::intToString(fd) + ", truncating to 510 bytes + CRLF")

class Client {
private:
    int fd;
    bool registered;
    bool authenticated;
    bool nickSet;
    bool userSet;
    std::string IPAddress;
    std::string nickname;
    std::string username;
    std::string hostname;
    std::string realname;
    std::string commandBuffer;
    bool greeted;

    Client(const Client& other);
    Client& operator=(const Client& other);

    std::string formatReply(const std::string& reply);
    void handleSendResult(ssize_t bytesSent, const std::string& formattedReply);

public:
    Client();
    ~Client();

    int getFd() const;
    std::string getIPAddress() const;
    std::string getNickname() const;
    std::string getUsername() const;
    std::string getHostname() const;
    std::string getRealname() const;
    void setRealname(const std::string& realname);
    bool isRegistered() const;
    bool isAuthenticated() const;
    bool isNickSet() const;
    bool isUserSet() const;
    std::string& getCommandBuffer();

    void setFd(int fd);
    void setIPAddress(const std::string& ipAddress);
    void setNickname(const std::string& nickname);
    void setUsername(const std::string& username);
    void setHostname(const std::string& hostname);
    void setAuthenticated(bool status);
    void setRegistered(bool status);
    void setNickSet(bool status);
    void setUserSet(bool status);

    bool isGreeted() const;
    void setGreeted(bool greeted);

    void appendToCommandBuffer(const std::string& data);
    void sendReply(const std::string& reply);
    static void sendWelcomeHowTo(int fd);
};
