#include "Command.hpp"
#include "Utils.hpp"
#include "Logger.hpp"

void Server::handleUser(std::list<std::string> cmdList, Client* client, Server* server) {
    if (cmdList.size() != 5) { // Expect username, mode, unused, realname
        client->sendReply(":ircserv 461 * USER :Not enough parameters\r\n"); // RFC 2812: ERR_NEEDMOREPARAMS
        return;
    }
    if (client->isRegistered()) {
        client->sendReply(":ircserv 462 " + client->getNickname() + " :Unauthorized command (already registered)\r\n");
        return;
    }
    if (!client->isAuthenticated()) {
        client->sendReply(":ircserv 462 * :You must send PASS before USER\r\n");
        return;
    }
    if (!client->isNickSet()) {
        client->sendReply(":ircserv 462 * :You must send NICK before USER\r\n");
        return;
    }

    std::list<std::string>::iterator it = cmdList.begin();
    ++it; // Skip "USER"
    std::string username = *it;
    ++it; // Mode
    ++it; // Unused
    ++it; // Realname
    std::string realname = *it;

    // Check for duplicate username (case-insensitive)
    for (std::map<int, Client*>::iterator it = clients.begin(); it != clients.end(); ++it) {
        if (it->second != client && Utils::toLower(it->second->getUsername()) == Utils::toLower(username)) {
            client->sendReply(":ircserv 462 * " + username + " :Username is already in use\r\n");
            return;
        }
    }

    client->setUsername(username);
    client->setHostname(client->getIPAddress());
    client->setUserSet(true); // Use the setter
    if (client->isAuthenticated() && client->isNickSet() && client->isUserSet()) {
        client->setRegistered(true);
        client->sendReply(":ircserv 001 " + client->getNickname() + " :Welcome to the IRC server!\r\n");
        client->sendReply(":ircserv 002 " + client->getNickname() + " :Your host is ircserv, running version 1.0\r\n");
        client->sendReply(":ircserv 003 " + client->getNickname() + " :This server was created on " + server->getCreatedTime() + "\r\n");
    }
    // RFC 2812 Reference: Section 4.1.3 - USER <username> <mode> <unused> :<realname>
}
