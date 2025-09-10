#include "Command.hpp"
#include "Utils.hpp"
#include "Logger.hpp"

void Server::handleNick(int fd, std::list<std::string> cmdList, Client* client) {
    (void)fd; // Suppress unused parameter warning if not used
    if (cmdList.size() != 2) {
        client->sendReply(":ircserv 431 * :No nickname given\r\n"); // RFC 2812: ERR_NONICKNAMEGIVEN
        return;
    }
    std::list<std::string>::iterator it = cmdList.begin();
    ++it; // Skip "NICK"
    std::string nick = *it;
    if (nick.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789[]\\`_^{|}") != std::string::npos) {
        client->sendReply(":ircserv 432 * " + nick + " :Erroneous nickname\r\n"); // RFC 2812: ERR_ERRONEUSNICKNAME
        return;
    }
    // Placeholder for duplicate check (implement in Server if needed)
    client->setNickname(nick);
    client->sendReply(":ircserv NOTICE " + nick + " :Nickname set to " + nick + "\r\n");
    // RFC 2812 Reference: Section 4.1.2 - NICK <nickname>
}
