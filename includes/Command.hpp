#pragma once
#include "Server.hpp"
#include "Client.hpp"
#include <list>
#include <string>

class Server;
class Channel;

// Common utility functions to eliminate duplication across command files
namespace CommandUtils {
    // Validation functions
    bool validateClientRegistration(Client* client);
    bool validateParameters(std::list<std::string>& cmdList, Client* client, const std::string& command, size_t minParams);
    
    // Common getters
    Channel* getChannel(Server* server, const std::string& channelName, Client* client);
    Client* getTargetClient(Server* server, Client* sender, const std::string& targetNick);
    
    // Helper functions
    std::string getNicknameOrDefault(Client* client, const std::string& defaultNick = "*");
}
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
void handlePing(std::list<std::string> cmdList, Client* client, Server* server);
void handleQuit(std::list<std::string> cmdList, Client* client, Server* server);
