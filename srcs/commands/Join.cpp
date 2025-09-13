#include "Command.hpp"
#include "Utils.hpp"
#include "Logger.hpp"
#include "Replies.hpp"
#include "Channel.hpp"

// RFC 2812: Section 3.2.1 - Validates channel name according to IRC protocol rules
static bool isValidChannelName(const std::string& name) {
    if (name.empty() || name.length() > 50 || name[0] != '#') { // Max 50 chars per RFC 2812
        return false;
    }
    for (size_t i = 0; i < name.length(); ++i) {
        if (name[i] == ' ' || name[i] == ',' || name[i] == '\7') {
            return false;
        }
    }
    return name.length() > 1; // Ensure non-empty name after '#'
}

// RFC 2812: Section 3.2.1 - Checks if JOIN command has valid parameters and client registration
static bool validateJoinParameters(std::list<std::string>& cmdList, Client* client) {
    if (cmdList.size() < 2) {
        std::string nickname;
        if (client->getNickname().empty()) {
            nickname = "*";
        } else {
            nickname = client->getNickname();
        }
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_NEEDMOREPARAMS + " " +
                          nickname + " JOIN :Not enough parameters");
        return false;
    }
    return true;
}

// RFC 2812: Section 3.2.1 - Verifies if client is registered before joining
static bool validateClientRegistration(Client* client) {
    if (!client->isRegistered()) {
        std::string nickname;
        if (client->getNickname().empty()) {
            nickname = "*";
        } else {
            nickname = client->getNickname();
        }
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_NOTREGISTERED + " " +
                          nickname + " :You have not registered");
        return false;
    }
    return true;
}

// RFC 2812: Section 3.2.1 - Creates or retrieves a channel, handling membership
static Channel* getOrCreateChannel(const std::string& channelName, Client* client, Server* server) {
    std::map<std::string, Channel*>& channels = server->getChannels();
    Channel* channel = NULL;
    std::map<std::string, Channel*>::iterator it = channels.find(channelName);
    if (it == channels.end()) {
        channel = new Channel(channelName, client);
        channels[channelName] = channel;
    } else {
        channel = it->second;
    }
    return channel;
}

// RFC 2812: Section 3.2.1 - Checks if client is already a member of the channel
static bool checkExistingMembership(Channel* channel, Client* client) {
    if (channel->isMember(client)) {
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_USERONCHANNEL + " " +
                          client->getNickname() + " " + client->getNickname() + " " +
                          channel->getName() + " :is already on channel");
        return true;
    }
    return false;
}

// RFC 2812: Section 3.2.1 - Broadcasts JOIN message and sends topic/replies to client
static void sendJoinMessages(const std::string& channelName, Channel* channel, Client* client) {
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
}

// RFC 2812: Section 3.2.1 - Processes a single channel join operation
static void processSingleJoin(const std::string& channelName, const std::string& key, Client* client, Server* server) {
    (void)key; // Suppress unused parameter warning, to be used later for key handling
    if (!isValidChannelName(channelName)) {
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_BADCHANMASK + " " +
                          client->getNickname() + " " + channelName + " :Invalid channel name");
        return;
    }

    Channel* channel = getOrCreateChannel(channelName, client, server);
    if (checkExistingMembership(channel, client)) {
        return;
    }

    channel->addMember(client);
    sendJoinMessages(channelName, channel, client);
    Logger::info(client->getNickname() + " joined " + channelName);
}

// RFC 2812: Section 3.2.1 - Handles JOIN command by processing channel and key lists
void handleJoin(std::list<std::string> cmdList, Client* client, Server* server) {
    if (!validateJoinParameters(cmdList, client) || !validateClientRegistration(client)) {
        return;
    }

    std::list<std::string>::iterator it = cmdList.begin();
    ++it; // Skip "JOIN"
    std::list<std::string> channels = Utils::split(*it, ',');
    std::list<std::string> keys;
    if (++it != cmdList.end()) {
        keys = Utils::split(*it, ',');
    }

    std::list<std::string>::iterator keyIt = keys.begin();
    for (std::list<std::string>::iterator chanIt = channels.begin(); chanIt != channels.end(); ++chanIt) {
        std::string key = (keyIt != keys.end()) ? *keyIt++ : "";
        processSingleJoin(*chanIt, key, client, server);
    }
}
