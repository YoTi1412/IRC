#pragma once

#include "Includes.hpp"
#define BUFFER_SIZE 1024

class Client; // Forward declaration

class Server {
private:
    std::string                     name;
    int                             port;
    std::string                     password;
    static bool                     signal;
    int                             sock_fd;
    struct sockaddr_in              serverAddress;
    std::string                     createdtime;
    std::vector<pollfd>             pollFds;
    std::map<int, Client*>          clients;
    std::set<int>                   processedFds; // Track processed fds

    // Helper methods for serverInit
    void createSocket();
    void configureServerAddress();
    void setSocketTimeout();
    void bindSocket();
    void listenOnSocket();
    void initPollFd();

    // Helper methods for serverRun
    void pollForEvents(std::vector<pollfd>& pollCopy);
    void acceptNewConnection();
    void handleClientData(std::vector<pollfd>::iterator& it);
    void cleanupDisconnectedClient(std::vector<pollfd>::iterator& it);

    Server(const Server &server);
    Server &operator=(const Server &server);
    void executeCommand(int fd, std::list<std::string> cmdList);
    void handlePass(std::list<std::string> cmdList, Client* client, Server* server);
    void handleNick(int fd, std::list<std::string> cmdList, Client* client);
    void handleUser(int fd, std::list<std::string> cmdList, Client* client);

public:
    Server(const std::string &port, const std::string &password);
    ~Server();

    void serverRun();
    void serverInit();
    static void sigHandler(int sig);

    const std::string &getName() const;
    const std::string &getCreatedTime() const;
    const std::string &getPassword() const; // Added getter for password
};

// Inline implementation of getPassword
inline const std::string &Server::getPassword() const {
    return password;
}
