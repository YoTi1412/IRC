#include "Command.hpp"
#include "Utils.hpp"
#include "Logger.hpp"
#include "Replies.hpp"
#include "Channel.hpp"

static bool isValidChannelName(const std::string& name) {
    if (name.empty() || name.length() > 50 || name[0] != '#') { // RFC 2812 max 50 chars
        return false;
    }
    for (size_t i = 0; i < name.length(); ++i) {
        if (name[i] == ' ' || name[i] == ',' || name[i] == '\7') {
            return false;
        }
    }
    return name.length() > 1; // Ensure non-empty name after '#'
}

static bool validateJoin(std::list<std::string>& cmdList, Client* client) {
    if (cmdList.size() < 2) {
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_NEEDMOREPARAMS + " " +
                          (client->getNickname().empty() ? "*" : client->getNickname()) +
                          " JOIN :Not enough parameters");
        return false;
    }
    if (!client->isRegistered()) {
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_NOTREGISTERED + " " +
                          (client->getNickname().empty() ? "*" : client->getNickname()) +
                          " :You have not registered");
        return false;
    }
    return true;
}

static void processJoin(const std::string& channelName, const std::string& key, Client* client, Server* server) {
    (void)key; // Suppress unused parameter warning, to be used later for key handling
    if (!isValidChannelName(channelName)) {
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_BADCHANMASK + " " +
                          client->getNickname() + " " + channelName + " :Invalid channel name");
        return;
    }

    std::map<std::string, Channel*>& channels = server->getChannels();
    Channel* channel = NULL;
    std::map<std::string, Channel*>::iterator it = channels.find(channelName);
    if (it == channels.end()) {
        channel = new Channel(channelName, client);
        channels[channelName] = channel;
        // Logging moved to Channel constructor, avoid duplicate here
    } else {
        channel = it->second;
        if (channel->isMember(client)) {
            client->sendReply(std::string(IRC_SERVER) + " " + ERR_USERONCHANNEL + " " +
                                          client->getNickname() + " " + client->getNickname() + " " +
                                          channelName + " :is already on channel");
            return;
        }
        channel->addMember(client);
    }

    std::string userIdent = client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname();
    std::string joinMsg = ":" + userIdent + " JOIN " + channelName;
    channel->broadcast(joinMsg, NULL);

    if (!channel->getTopic().empty()) {
        client->sendReply(std::string(IRC_SERVER) + " " + RPL_TOPIC + " " +
                          client->getNickname() + " " + channelName + " :" + channel->getTopic());
    }

    client->sendReply(std::string(IRC_SERVER) + " " + RPL_NAMREPLY + " " +
                      client->getNickname() + " = " + channelName + " :" + channel->getMemberList());
    client->sendReply(std::string(IRC_SERVER) + " " + RPL_ENDOFNAMES + " " +
                      client->getNickname() + " " + channelName + " :End of NAMES list");

    Logger::info(client->getNickname() + " joined " + channelName);
}

void handleJoin(std::list<std::string> cmdList, Client* client, Server* server) {
    if (!validateJoin(cmdList, client)) return;

    std::list<std::string>::iterator it = cmdList.begin();
    ++it; // Skip "JOIN"
    std::string channelsStr = *it;
    std::list<std::string> channels = Utils::split(channelsStr, ',');
    std::list<std::string> keys;
    if (++it != cmdList.end()) {
        keys = Utils::split(*it, ',');
    }

    std::list<std::string>::iterator keyIt = keys.begin();
    for (std::list<std::string>::iterator chanIt = channels.begin(); chanIt != channels.end(); ++chanIt) {
        std::string key = (keyIt != keys.end()) ? *keyIt++ : "";
        processJoin(*chanIt, key, client, server);
    }
}
