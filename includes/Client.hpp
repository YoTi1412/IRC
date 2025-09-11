#pragma once
#include "Includes.hpp"
#define CRLF "\r\n"

class Client {
private:
    int fd;
    std::string IPAddress;
    std::string nickname;
    std::string username;
    std::string hostname;
    bool registered;
    bool authenticated;
    std::string commandBuffer;
    bool nickSet;  // New flag to track if NICK is set
    bool userSet;  // New flag to track if USER is set

public:
    Client();
    ~Client();

    int getFd() const;
    std::string getIPAddress() const;
    std::string getNickname() const;
    std::string getUsername() const;
    std::string getHostname() const;
    bool isRegistered() const;
    bool isAuthenticated() const;
    bool isNickSet() const;
    bool isUserSet() const;

    void setAuthenticated();
    void setRegistered();
    void setFd(int fd);
    void setIPAddress(const std::string& IPAddress);
    void setNickname(const std::string& nickname);
    void setHostname(const std::string& hostname);
    void setUsername(const std::string& username);
    void setRegistered(bool status);
    void setNickSet(bool nickSet);  // Removed const
    void setUserSet(bool userSet);  // Removed const

    void processCommand(const std::string& command);
    void sendReply(const std::string& reply);
    std::string& getCommandBuffer();
};
