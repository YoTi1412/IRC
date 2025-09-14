#pragma once
#include "Server.hpp"
#include "Client.hpp"
#include <list>
#include <string>

class Server;
// Forward declarations for command handlers
void handlePass(std::list<std::string> cmdList, Client* client, Server* server);
void handleNick(std::list<std::string> cmdList, Client* client, Server* server);
void handleUser(std::list<std::string> cmdList, Client* client, Server* server);
void handleJoin(std::list<std::string> cmdList, Client* client, Server* server);
void handlePrivmsg(std::list<std::string> cmdList, Client* client, Server* server);
void handlePart(std::list<std::string> cmdList, Client* client, Server* server);
void handleMode(std::list<std::string> cmdList, Client* client, Server* server);
void handleInvite(std::list<std::string> cmdList, Client* client, Server* server);
void handleTopic(std::list<std::string> cmdList, Client* client, Server* server);
void handleKick(std::list<std::string> cmdList, Client* client, Server* server);
void handleNames(std::list<std::string> cmdList, Client* client, Server* server);