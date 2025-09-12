#include "Command.hpp"
#include "Utils.hpp"
#include "Logger.hpp"
#include "Replies.hpp"
#include <sstream>

static bool validatePrivmsg(std::list<std::string>& cmdList, Client* client) {
    if (cmdList.size() < 2) {
        std::ostringstream oss;
        oss << IRC_SERVER << " " << ERR_NEEDMOREPARAMS << " " << client->getNickname() << " PRIVMSG :Not enough parameters";
        client->sendReply(oss.str());
        return false;
    }
    if (!client->isRegistered()) {
        std::ostringstream oss;
        oss << IRC_SERVER << " " << ERR_NOTREGISTERED << " " << client->getNickname() << " :You have not registered";
        client->sendReply(oss.str());
        return false;
    }
    return true;
}

void handlePrivmsg(std::list<std::string> cmdList, Client* client, Server* server) {
    if (!validatePrivmsg(cmdList, client)) return;

    std::list<std::string>::iterator it = cmdList.begin();
    ++it; // Skip "PRIVMSG"
    std::string target = *it;
    ++it;
    if (it == cmdList.end()) {
        std::ostringstream oss;
        oss << IRC_SERVER << " " << ERR_NEEDMOREPARAMS << " " << client->getNickname() << " PRIVMSG :No message provided";
        client->sendReply(oss.str());
        return;
    }
    std::string message = *it; // Simple case, assumes single param for now
    while (++it != cmdList.end()) {
        message += " " + *it; // Concatenate multi-word messages
    }

    std::string userIdent = client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname();
    std::string fullMsg = ":" + userIdent + " PRIVMSG " + target + " :" + message;

    if (target[0] == '#') { // Channel message
        std::map<std::string, Channel*>& channels = server->getChannels();
        std::map<std::string, Channel*>::iterator chanIt = channels.find(target);
        if (chanIt == channels.end()) {
            std::ostringstream oss;
            oss << IRC_SERVER << " " << ERR_NOSUCHCHANNEL << " " << client->getNickname() << " " << target << " :No such channel";
            client->sendReply(oss.str());
            return;
        }
        Channel* channel = chanIt->second;
        if (!channel->isMember(client)) {
            std::ostringstream oss;
            oss << IRC_SERVER << " " << ERR_NOTONCHANNEL << " " << client->getNickname() << " " << target << " :You're not on that channel";
            client->sendReply(oss.str());
            return;
        }
        channel->broadcast(fullMsg, client); // Exclude sender
    } else { // User message
        std::map<int, Client*>& clients = server->getClients();
        bool found = false;
        for (std::map<int, Client*>::iterator clientIt = clients.begin(); clientIt != clients.end(); ++clientIt) {
            if (clientIt->second->getNickname() == target) {
                clientIt->second->sendReply(fullMsg);
                found = true;
                break;
            }
        }
        if (!found) {
            std::ostringstream oss;
            oss << IRC_SERVER << " " << ERR_NOSUCHNICK << " " << client->getNickname() << " " << target << " :No such nick";
            client->sendReply(oss.str());
        }
    }

    Logger::info(client->getNickname() + " sent PRIVMSG to " + target + ": " + message);
}