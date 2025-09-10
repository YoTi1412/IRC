#pragma once

#include "Includes.hpp"
#include "Server.hpp"
#include "Client.hpp"

class Server;

// Forward declarations for command handlers
void handlePass(int fd, std::list<std::string> cmdList, Client* client, Server* server);
void handleNick(int fd, std::list<std::string> cmdList, Client* client);
void handleUser(int fd, std::list<std::string> cmdList, Client* client);
