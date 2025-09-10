#include "Command.hpp"
#include "Utils.hpp"
#include "Logger.hpp"

void Server::handleUser(std::list<std::string> cmdList, Client* client, Server* server) {
    if (cmdList.size() != 5) {
        client->sendReply(":ircserv 461 * USER :Not enough parameters\r\n"); // RFC 2812: ERR_NEEDMOREPARAMS
        return;
    }
    std::list<std::string>::iterator it = cmdList.begin();
    ++it; // Skip "USER"
    std::string username = *it;
    ++it; // Mode
    ++it; // Unused
    ++it; // Realname (starts with :)
    std::string realname = it->substr(it->find(":") + 1); // Extract realname

    client->setUsername(username);
    client->setHostname(client->getIPAddress()); // Now works with const std::string&
    if (client->isAuthenticated() && !client->getNickname().empty()) {
        client->setRegistered(true);
        // Send welcome messages
        client->sendReply(":ircserv 001 " + client->getNickname() + " :Welcome to the IRC server!\r\n");
        client->sendReply(":ircserv 002 " + client->getNickname() + " :Your host is ircserv, running version 1.0\r\n");
        client->sendReply(":ircserv 003 " + client->getNickname() + " :This server was created on " + server->getCreatedTime() + "\r\n");
    }
    // RFC 2812 Reference: Section 4.1.3 - USER <username> <mode> <unused> :<realname>
}
