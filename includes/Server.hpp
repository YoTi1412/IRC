#pragma once

#include "Includes.hpp"

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
        // std::map<std::string, Channel*> channels;

        Server(const Server &server);
        Server &operator=(const Server &server);

    public:
        Server(const std::string &port, const std::string &password);
        ~Server();

        void serverRun();
        void serverInit();
        static void sigHandler(int sig);

        const std::string &getName() const;
        const std::string &getCreatedTime() const;

};
