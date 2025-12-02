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

static bool checkMembership(Channel* channel, Client* client) {
    if (!channel->isMember(client)) {
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_NOTONCHANNEL + " " +
                          client->getNickname() + " " + channel->getName() + " :You're not on that channel");
        return false;
    }
    return true;
}

static void sendPartMessage(const std::string& channelName, const std::string& message, Client* client, Channel* channel) {
    std::string userIdent = client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname();
    std::string partMsg = ":" + userIdent + " PART " + channelName;
    if (!message.empty()) {
        partMsg += " :" + message;
    }
    channel->broadcast(partMsg, NULL);
}

static void removeClientFromChannel(const std::string& channelName, Client* client, Channel* channel, Server* server) {
    channel->removeMember(client);
    if (channel->getMemberCount() == 0) {
        delete channel;
        server->getChannels().erase(channelName);
        Logger::info("Channel " + channelName + " deleted as it became empty");
    }
    Logger::info(client->getNickname() + " parted " + channelName);
}

static void processSinglePart(const std::string& channelName, const std::string& message, Client* client, Server* server) {
    if (!isValidChannelName(channelName)) {
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_NOSUCHCHANNEL + " " +
                          client->getNickname() + " " + channelName + " :No such channel");
        return;
    }

    Channel* channel = CommandUtils::getChannel(server, channelName, client);
    if (channel == NULL) return;

    if (!checkMembership(channel, client)) return;

    sendPartMessage(channelName, message, client, channel);
    removeClientFromChannel(channelName, client, channel, server);
}

static std::string extractPartMessage(std::list<std::string>::iterator it, std::list<std::string>& cmdList) {
    std::string message;
    if (it != cmdList.end()) {
        message = *it;
        while (++it != cmdList.end()) {
            message += " " + *it;
        }
        if (!message.empty() && message[0] == ':') {
            message = message.substr(1);
        }
    }
    return message;
}

static bool handleMissingParams(const std::string& channelsStr, Client* client) {
    if (channelsStr.empty() || channelsStr[0] == ':') {
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_NEEDMOREPARAMS + " " +
                          client->getNickname() + " PART :Not enough parameters");
        return true;
    }
    return false;
}

void handlePart(std::list<std::string> cmdList, Client* client, Server* server) {
    if (!CommandUtils::validateClientRegistration(client)) return;

    if (cmdList.size() < 2) {
        std::string nickname = client->getNickname().empty() ? "*" : client->getNickname();
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_NEEDMOREPARAMS + " " +
                          nickname + " PART :Not enough parameters");
        return;
    }

    std::list<std::string>::iterator it = cmdList.begin();
    ++it;
    std::string channelsStr = *it;

    if (handleMissingParams(channelsStr, client)) return;

    ++it;
    std::string message = extractPartMessage(it, cmdList);

    std::list<std::string> channels = Utils::split(channelsStr, ',');
    for (std::list<std::string>::iterator chanIt = channels.begin(); chanIt != channels.end(); ++chanIt) {
        processSinglePart(*chanIt, message, client, server);
    }
}
