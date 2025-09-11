#include "Command.hpp"
#include "Utils.hpp"
#include "Logger.hpp"

/**
 * Handles the USER command as specified in RFC 2812 (Section 3.1.3).
 *
 * Syntax:
 *   USER <username> <mode> <unused> :<realname>
 *
 * - <username>: Sets the username of the client.
 * - <mode>: User mode (ignored, server uses client’s actual connection data).
 * - <unused>: Historical field for server name (ignored).
 * - <realname>: Full name / description of the user.
 *
 * Behavior:
 * - If fewer than 4 parameters are supplied, reply with ERR_NEEDMOREPARAMS (461).
 * - The hostname is taken from the client’s IP address, not the supplied parameter.
 * - Registration completes when PASS, NICK, and USER are all successfully set.
 * - Upon successful registration, the server sends:
 *      • RPL_WELCOME (001)
 *      • RPL_YOURHOST (002)
 *      • RPL_CREATED (003)
 *
 * Reference: RFC 2812 Section 3.1.3
 */
void handleUser(std::list<std::string> cmdList, Client* client, Server* server) {
    if (cmdList.size() != 5) {
        client->sendReply(":ircserv 461 * USER :Not enough parameters\r\n");
        return;
    }

    std::list<std::string>::iterator it = cmdList.begin();
    ++it; // Skip "USER"
    std::string username = *it;
    ++it; // Mode (ignored)
    ++it; // Unused (servername, ignored)
    ++it; // Realname (starts with :)
    std::string realname = it->substr(it->find(":") + 1);

    client->setUsername(username);
    client->setHostname(client->getIPAddress());

    if (client->isAuthenticated() && !client->getNickname().empty()) {
        client->setRegistered(true);
        client->sendReply(":ircserv 001 " + client->getNickname() + " :Welcome to the IRC server!\r\n");
        client->sendReply(":ircserv 002 " + client->getNickname() + " :Your host is ircserv, running version 1.0\r\n");
        client->sendReply(":ircserv 003 " + client->getNickname() + " :This server was created on " + server->getCreatedTime() + "\r\n");
    }
}
