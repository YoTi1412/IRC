#include "Command.hpp"
#include "Utils.hpp"
#include "Logger.hpp"
#include "Replies.hpp"

void handleJoin(std::list<std::string> cmdList, Client* client) {
    if (cmdList.size() < 2) {
        client->sendReply(":ircserv " ERR_NEEDMOREPARAMS " " + client->getNickname() + " JOIN :Not enough parameters\r\n");
        return;
    }
    std::list<std::string>::iterator it = cmdList.begin();
    ++it; // Skip "JOIN"
    std::string channel = *it;

    if (channel.empty() || channel[0] != '#') {
        client->sendReply(":ircserv " ERR_NOSUCHCHANNEL " " + client->getNickname() + " " + channel + " :No such channel\r\n");
        return;
    }
    // Simple channel join (placeholder - add channel management later)
    client->sendReply(":ircserv NOTICE " + client->getNickname() + " :Joined channel " + channel + "\r\n");
    Logger::info(client->getNickname() + " joined channel " + channel);
}
