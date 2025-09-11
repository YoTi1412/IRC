#pragma once

#include "Includes.hpp"
#include "Replies.hpp"

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

    // Helper methods for setup
    void createSocket();
    void configureServerAddress();
    void setSocketTimeout();
    void bindSocket();
    void listenOnSocket();
    void initPollFd();

    // Helper methods for run
    void initializeFileDescriptors();
    void pollForEvents(std::vector<pollfd>& pollCopy);
    void processServerSocket();
    void processClientSockets();
    void acceptNewConnection();
    void handleClientData(std::vector<pollfd>::iterator& it);
    void cleanupDisconnectedClient(std::vector<pollfd>::iterator& it); // Updated: Removed bytesRead

    Server(const Server &server);
    Server &operator=(const Server &server);
    void executeCommand(int fd, std::list<std::string> cmdList);
    void handleJoin(std::list<std::string> cmdList, Client* client);
    void handlePass(std::list<std::string> cmdList, Client* client, Server* server);
    void handleNick(std::list<std::string> cmdList, Client* client);
    void handleUser(std::list<std::string> cmdList, Client* client, Server* server);

public:
    Server(const std::string &port, const std::string &password);
    ~Server();

    void serverInit(); // Updated: Changed from setup to serverInit
    void serverRun();  // Updated: Changed from run to serverRun
    static void sigHandler(int sig);

    const std::string &getName() const;
    const std::string &getCreatedTime() const;
    const std::string &getPassword() const;
};
