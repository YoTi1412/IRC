#pragma once

#include "Includes.hpp"
#include "Replies.hpp"
#include "Command.hpp"
#include "Channel.hpp"
#include <sys/epoll.h>
#include <sys/resource.h>

#define BUFFER_SIZE 1024
#define MAX_EVENTS 1000

class Client;

class Server {
private:
    std::string                     name;
    int                             port;
    std::string                     password;
    static bool                     signal;
    int                             sock_fd;
    int                             epfd;
    struct sockaddr_in              serverAddress;
    std::string                     createdtime;
    std::map<int, Client*>          clients;
    std::set<int>                   processedFds;
    std::map<std::string, Channel*> channels;

    void createSocket();
    void configureServerAddress();
    void bindSocket();
    void listenOnSocket();
    void initEpoll();
    void increaseFdLimit();
    void logInitialization();
    void validateArgs(const std::string &portStr, const std::string &password);
    void checkEmptyArgs(const std::string &portStr, const std::string &password);
    void validatePort(const std::string &portStr);
    void validatePassword(const std::string &password);
    void cleanupAllClients();
    void closeSocket();
    void logShutdown();

    void waitForEvents(struct epoll_event events[], int& nfds);
    void processEvents(struct epoll_event events[], int nfds);
    void handleClientEvent(int fd, uint32_t events);

    void acceptNewConnection();
    void handleAcceptResult(int clientFd, sockaddr_in& clientAddr);
    void configureNewClient(int clientFd, sockaddr_in& clientAddr);
    Client* createNewClient(int clientFd, sockaddr_in& clientAddr);
    void logNewConnection(int clientFd, const char* ip, int port);
    bool tryHandleHttpClient(int clientFd);
    void sendIrcGreeting(Client* client);
    void addClientToEpoll(int clientFd);

    void handleClientData(int fd);
    void processReadResult(int fd, char* buffer, int bytesRead);
    void handleReadError(int fd);
    void handleReadSuccess(int fd, char* buffer, int bytesRead);
    void appendToClientBuffer(int fd, const char* data);
    void processClientBuffer(int fd);
    bool hasNextMessage(const std::string& buffer);
    std::string extractNextMessage(std::string& buffer);
    std::string extractMessage(const std::string& buffer, size_t pos);
    std::list<std::string> parseMessage(const std::string& message);
    void tokenizePrefix(const std::string& prefix, std::list<std::string>& cmdList);

    void executeCommand(int fd, std::list<std::string> cmdList);
    void sendInvalidCommandError(int fd, const std::string& cmd);
    void dispatchCommand(const std::string& cmd, std::list<std::string> cmdList, Client* client);
    void sendUnknownCommandError(Client* client, const std::string& cmd);
    bool isUpperCase(const std::string& str);

    void sendHttpResponse(int fd);

    void cleanupAllChannels();

    Server(const Server &server);
    Server &operator=(const Server &server);

public:
    Server(const std::string &port, const std::string &password);
    ~Server();

    void serverInit();
    void serverRun();
    static void sigHandler(int sig);
    void setReuseAddr();

    const std::string &getName() const;
    const std::string &getCreatedTime() const;
    const std::string &getPassword() const;
    std::map<int, Client*>& getClients();

    std::map<std::string, Channel*>& getChannels();

    Client* getClientByNickname(const std::string& nickname) const;
    void removeChannel(const std::string& channelName);
    void handleClientDisconnect(int fd);
};
