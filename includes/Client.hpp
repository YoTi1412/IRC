#pragma once
#include "Includes.hpp"

class Client {
private:
    int fd;
    std::string IPAddress;
    std::string nickname;
    std::string username;
    bool registered; // Tracks if client is fully registered (PASS, NICK, USER)

public:
    Client();
    ~Client();

    // Getters
    int getFd() const;
    std::string getIPAddress() const;
    std::string getNickname() const;
    std::string getUsername() const;
    bool isRegistered() const;

    // Setters
    void setFd(int fd);
    void setIPAddress(const std::string& IPAddress);
    void setNickname(const std::string& nickname);
    void setUsername(const std::string& username);
    void setRegistered(bool status);

    // IRC command handling (placeholders)
    void processCommand(const std::string& command);
};
