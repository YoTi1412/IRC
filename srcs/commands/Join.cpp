#include "Includes.hpp"

static bool isValidChannelName(const std::string& name) {
    if (name.empty() || name.length() > 50 || name[0] != '#') {
        return false;
    }
    for (size_t i = 0; i < name.length(); ++i) {
        if (name[i] == ' ' || name[i] == ',' || name[i] == '\7') {
            return false;
        }
    }
    return name.length() > 1;
}

static bool validateJoinParameters(std::list<std::string>& cmdList, Client* client) {
    return CommandUtils::validateParameters(cmdList, client, "JOIN", 2);
}

static Channel* getOrCreateChannel(const std::string& channelName, Client* client, Server* server) {
    std::map<std::string, Channel*>& channels = server->getChannels();
    std::map<std::string, Channel*>::iterator chanIt = channels.find(channelName);
    if (chanIt == channels.end()) {
        Channel* newChannel = new Channel(channelName, client);
        channels[channelName] = newChannel;
        newChannel->addOperator(client->getFd());
        Logger::info("Channel " + channelName + " created by " + client->getNickname() + ", " +
                     client->getNickname() + " set as operator");
        return newChannel;
    }
    return chanIt->second;
}

static bool checkExistingMembership(Channel* channel, Client* client) {
    if (channel->isMember(client)) {
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_USERONCHANNEL + " " +
                          client->getNickname() + " " + channel->getName() + " :You are already on that channel\r\n");
        return true;
    }
    return false;
}

static bool validateChannelModes(Channel* channel, Client* client, const std::string& key) {
    if (channel->getInviteOnly() && !channel->isInvited(client->getFd())) {
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_INVITEONLYCHAN + " " +
                          client->getNickname() + " " + channel->getName() + " :Cannot join channel (+i)\r\n");
        Logger::warning(client->getNickname() + " failed to join " + channel->getName() + " due to +i restriction");
        return false;
    }
    if (channel->getKeyProtected()) {
        if (key.empty()) {
            client->sendReply(std::string(IRC_SERVER) + " " + ERR_BADCHANNELKEY + " " +
                              client->getNickname() + " " + channel->getName() + " :Key required (+k)\r\n");
            Logger::warning(client->getNickname() + " failed to join " + channel->getName() + " due to missing key");
            return false;
        }
        if (key != channel->getKey()) {
            client->sendReply(std::string(IRC_SERVER) + " " + ERR_BADCHANNELKEY + " " +
                              client->getNickname() + " " + channel->getName() + " :Incorrect key (+k)\r\n");
            Logger::warning(client->getNickname() + " failed to join " + channel->getName() + " due to wrong key");
            return false;
        }
    }
    if (channel->getLimited()) {
        if (channel->getLimit() == 0) {
            Logger::warning("Channel " + channel->getName() + " has limit 0, join denied");
            client->sendReply(std::string(IRC_SERVER) + " " + ERR_CHANNELISFULL + " " +
                              client->getNickname() + " " + channel->getName() + " :Channel limit is 0 (+l)\r\n");
            return false;
        }
        if (channel->getMemberCount() >= channel->getLimit()) {
            client->sendReply(std::string(IRC_SERVER) + " " + ERR_CHANNELISFULL + " " +
                              client->getNickname() + " " + channel->getName() + " :Cannot join channel (+l)\r\n");
            Logger::warning(client->getNickname() + " failed to join " + channel->getName() + " due to +l limit");
            return false;
        }
    }
    return true;
}

static void sendJoinMessages(const std::string& channelName, Channel* channel, Client* client) {
    std::string userIdent = client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname();
    std::string joinMsg = ":" + userIdent + " JOIN " + channelName + CRLF;
    channel->broadcast(joinMsg, NULL);

    if (!channel->getTopic().empty()) {
        client->sendReply(std::string(IRC_SERVER) + " " + RPL_TOPIC + " " +
                          client->getNickname() + " " + channelName + " :" + channel->getTopic() + CRLF);
        client->sendReply(std::string(":ircserv NOTICE ") + client->getNickname() + " :Topic set by " +
                          channel->getTopicSetter() + " at " + Utils::formatTime(channel->getTopicTime()) + CRLF);
    }

    client->sendReply(std::string(IRC_SERVER) + " " + RPL_NAMREPLY + " " +
                      client->getNickname() + " = " + channelName + " :" + channel->getMemberList() + CRLF);
    client->sendReply(std::string(IRC_SERVER) + " " + RPL_ENDOFNAMES + " " +
                      client->getNickname() + " " + channelName + " :End of NAMES list" + CRLF);
}

static void processSingleJoin(const std::string& channelName, const std::string& key, Client* client, Server* server) {
    if (!isValidChannelName(channelName)) {
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_BADCHANMASK + " " +
                          client->getNickname() + " " + channelName + " :Invalid channel name\r\n");
        return;
    }

    Channel* channel = getOrCreateChannel(channelName, client, server);
    if (checkExistingMembership(channel, client)) {
        return;
    }

    if (!validateChannelModes(channel, client, key)) {
        return;
    }

    channel->addMember(client);
    sendJoinMessages(channelName, channel, client);
    Logger::info(client->getNickname() + " joined " + channelName);
}

void handleJoin(std::list<std::string> cmdList, Client* client, Server* server) {
    if (!validateJoinParameters(cmdList, client) || !CommandUtils::validateClientRegistration(client)) {
        return;
    }

    std::list<std::string>::iterator it = cmdList.begin();
    ++it;
    std::list<std::string> channels = Utils::split(*it, ',');
    std::list<std::string> keys;
    if (++it != cmdList.end()) {
        keys = Utils::split(*it, ',');
    }

    std::list<std::string>::iterator keyIt = keys.begin();
    for (std::list<std::string>::iterator chanIt = channels.begin(); chanIt != channels.end(); ++chanIt) {
        std::string key;
        if (keyIt != keys.end()) {
            key = *keyIt++;
        } else {
            key = "";
        }
        processSingleJoin(*chanIt, key, client, server);
    }
}
