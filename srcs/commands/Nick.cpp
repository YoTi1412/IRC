#include "Command.hpp"
#include "Utils.hpp"
#include "Client.hpp"
#include "Logger.hpp"

void Server::handleNick(std::list<std::string> cmdList, Client* client) {
    if (cmdList.size() < 2) {
        client->sendReply(":ircserv 431 * :No nickname given\r\n"); // RFC 2812: ERR_NONICKNAMEGIVEN
        return;
    }
    // Only restrict NICK before registration for order
    if (!client->isAuthenticated() && client->isRegistered()) {
        client->sendReply(":ircserv 462 " + client->getNickname() + " :Unauthorized command (already registered)\r\n");
        return;
    }
    if (!client->isAuthenticated()) {
        client->sendReply(":ircserv 462 * :You must send PASS before NICK\r\n");
        return;
    }
    if (client->isUserSet() && !client->isRegistered()) {
        client->sendReply(":ircserv 462 * :NICK must be sent before USER\r\n");
        return;
    }

    std::list<std::string>::iterator it = cmdList.begin();
    ++it; // Skip "NICK"
    std::string nick = *it;
    if (nick.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789[]\\`_^{|}") != std::string::npos) {
        client->sendReply(":ircserv 432 * " + nick + " :Erroneous nickname\r\n"); // RFC 2812: ERR_ERRONEUSNICKNAME
        return;
    }
    // Check for duplicate nickname (case-insensitive)
    for (std::map<int, Client*>::iterator it = clients.begin(); it != clients.end(); ++it) {
        if (it->second != client && Utils::toLower(it->second->getNickname()) == Utils::toLower(nick)) {
            client->sendReply(":ircserv 433 * " + nick + " :Nickname is already in use\r\n");
            return;
        }
    }
    std::string oldNick = client->getNickname();
    client->setNickname(nick);
    client->setNickSet(true);
    if (!oldNick.empty() && client->isRegistered()) {
        client->sendReply(":" + oldNick + "!" + client->getUsername() + "@" + client->getHostname() + " NICK " + nick + "\r\n");
    }
    else {
        client->sendReply(":ircserv NOTICE " + nick + " :Nickname set to " + nick + "\r\n");
    }
    if (client->isAuthenticated() && client->isNickSet() && client->isUserSet() && !client->isRegistered()) {
        client->setRegistered(true);
        client->sendReply(":ircserv 001 " + client->getNickname() + " :Welcome to the IRC server!\r\n");
        client->sendReply(":ircserv 002 " + client->getNickname() + " :Your host is ircserv, running version 1.0\r\n");
        client->sendReply(":ircserv 003 " + client->getNickname() + " :This server was created on " + Utils::getFormattedTime() + "\r\n");
    }
    // RFC 2812 Reference: Section 3.1.2 - NICK <nickname>
}
