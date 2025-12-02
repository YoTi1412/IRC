#include "Includes.hpp"
#include "Channel.hpp"

static bool validateParameters(std::list<std::string>& cmdList, Client* client) {
    return CommandUtils::validateParameters(cmdList, client, "TOPIC", 2);
}

static bool checkExistingMembership(Channel* channel, Client* client) {
    if (!channel->isMember(client)) {
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_NOTONCHANNEL + " " +
                          client->getNickname() + " " + channel->getName() + " :You're not on that channel\r\n");
        return false;
    }
    return true;
}

void broadcastTopicChange(Channel* channel, Client* client, const std::string& channelName, const std::string& topic) {
    std::ostringstream oss;
    oss << ":" << client->getNickname() << "!" << client->getUsername()
        << "@" << client->getHostname()
        << " TOPIC " << channelName << " :" << topic;
    std::string message = oss.str();
    if (topic.empty()) {
        std::string::size_type pos = message.find_last_not_of(" \t");
        if (pos != std::string::npos) {
            message.erase(pos + 1);
        }
    }
    channel->broadcast(message + CRLF, NULL);
}

static void handleTopicView(Channel* channel, Client* client) {
    if (channel->getTopic().empty()) {
        client->sendReply(std::string(IRC_SERVER) + " " + RPL_NOTOPIC + " " +
                          client->getNickname() + " " + channel->getName() + " :No topic is set\r\n");
    } else {
        client->sendReply(std::string(IRC_SERVER) + " " + RPL_TOPIC + " " +
                          client->getNickname() + " " + channel->getName() + " :" + channel->getTopic() + CRLF);
        client->sendReply(std::string(":ircserv NOTICE ") + client->getNickname() + " :Topic set by " +
                          channel->getTopicSetter() + " at " + Utils::formatTime(channel->getTopicTime()) + CRLF);
    }
}

static bool processTopicSet(std::list<std::string>& cmdList, std::list<std::string>::iterator& it, Channel* channel, Client* client) {
    if (cmdList.size() > 3) {
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_NEEDMOREPARAMS + " " +
                          client->getNickname() + " TOPIC :Not enough parameters\r\n");
        return false;
    }

    std::string topic = *it;
    if (!topic.empty() && topic[0] == ':') {
        topic = topic.substr(1);
    }

    size_t start = topic.find_first_not_of(" \t");
    if (start == std::string::npos) {
        topic = "";
    } else {
        topic = topic.substr(start);
        size_t end = topic.find_last_not_of(" \t");
        topic = (end == std::string::npos) ? "" : topic.substr(0, end + 1);
    }

    if (channel->getTopicRestricted() && !channel->isOperator(client)) {
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_CHANOPRIVSNEEDED + " " +
                          client->getNickname() + " " + channel->getName() + " :You're not channel operator\r\n");
        return false;
    }

    channel->setTopic(topic, client);
    return true;
}

void handleTopic(std::list<std::string> cmdList, Client* client, Server* server) {
    if (!validateParameters(cmdList, client) || !CommandUtils::validateClientRegistration(client)) {
        return;
    }

    std::list<std::string>::iterator it = cmdList.begin();
    ++it;
    std::string channelName = *it;

    Channel* channel = CommandUtils::getChannel(server, channelName, client);
    if (!channel || !checkExistingMembership(channel, client)) {
        return;
    }

    ++it;
    if (it == cmdList.end()) {
        handleTopicView(channel, client);
        return;
    }

    if (processTopicSet(cmdList, it, channel, client)) {
        broadcastTopicChange(channel, client, channel->getName(), channel->getTopic());
    }
}
