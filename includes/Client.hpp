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

public:
    Client();
    ~Client();

    int getFd() const;
    std::string getIPAddress() const;
    std::string getNickname() const;
    std::string getUsername() const;
    bool isAuthenticated() const; // Added getter for authenticated status
    void setAuthenticated(); // Renamed isAuthenticated to setAuthenticated for clarity
    bool isRegistered() const;
    void setRegistered();

    void setFd(int fd);
    void setIPAddress(const std::string& IPAddress);
    void setNickname(const std::string& nickname);
    void setHostname(const std::string& hostname); // Changed to const reference
    void setUsername(const std::string& username);
    void setRegistered(bool status);

    // IRC command handling
    void processCommand(const std::string& command);

    // Send a formatted IRC reply to the client
    // RFC 2812 Reference: Section 2.3 - Message Format
    void sendReply(const std::string& reply);

    // Access command buffer
    std::string& getCommandBuffer();
};
