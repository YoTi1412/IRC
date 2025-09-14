#include "Command.hpp"
#include "Utils.hpp"
#include "Logger.hpp"
#include "Replies.hpp"
#include "Channel.hpp"

/**
 * @brief Validates the TOPIC command parameters per RFC 2812 §4.2.4.
 * Ensures at least the channel name is provided; returns false with ERR_NEEDMOREPARAMS (461) if insufficient.
 */
static bool validateParameters(std::list<std::string>& cmdList, Client* client) {
    if (cmdList.size() < 2) {
        std::string nickname = client->getNickname().empty() ? "*" : client->getNickname();
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_NEEDMOREPARAMS + " " +
                          nickname + " TOPIC :Not enough parameters\r\n");
        return false;
    }
    return true;
}

/**
 * @brief Validates client registration status per RFC 2812 §3.1.
 * Ensures the client is registered; returns false with ERR_NOTREGISTERED (451) if not.
 */
static bool validateClientRegistration(Client* client) {
    if (!client->isRegistered()) {
        std::string nickname = client->getNickname().empty() ? "*" : client->getNickname();
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_NOTREGISTERED + " " +
                          nickname + " :You have not registered\r\n");
        return false;
    }
    return true;
}

/**
 * @brief Retrieves the channel object for the given name per RFC 2812 §4.2.4.
 * Returns NULL and sends ERR_NOSUCHCHANNEL (403) if the channel does not exist.
 */
static Channel* getChannel(Server* server, const std::string& channelName, Client* client) {
    std::map<std::string, Channel*>::iterator it = server->getChannels().find(channelName);
    if (it == server->getChannels().end()) {
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_NOSUCHCHANNEL + " " +
                          client->getNickname() + " " + channelName + " :No such channel\r\n");
        return NULL;
    }
    return it->second;
}

/**
 * @brief Checks if the client is a member of the channel per RFC 2812 §4.2.4.
 * Returns false and sends ERR_NOTONCHANNEL (442) if the client is not a member.
 */
static bool checkExistingMembership(Channel* channel, Client* client) {
    if (!channel->isMember(client)) {
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_NOTONCHANNEL + " " +
                          client->getNickname() + " " + channel->getName() + " :You're not on that channel\r\n");
        return false;
    }
    return true;
}

/**
 * @brief Broadcasts the topic change to all channel members per RFC 2812 §4.2.4.
 * Constructs and sends the TOPIC message with proper prefix and trailing format.
 */
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
    channel->broadcast(message + "\r\n", NULL);
}

/**
 * @brief Handles viewing the current topic per RFC 2812 §4.2.4.
 * Sends RPL_NOTOPIC (331) if no topic, or RPL_TOPIC (332) with the topic, followed by a NOTICE with setter/time.
 */
static void handleTopicView(Channel* channel, Client* client) {
    if (channel->getTopic().empty()) {
        client->sendReply(std::string(IRC_SERVER) + " " + RPL_NOTOPIC + " " +
                          client->getNickname() + " " + channel->getName() + " :No topic is set\r\n");
    } else {
        client->sendReply(std::string(IRC_SERVER) + " " + RPL_TOPIC + " " +
                          client->getNickname() + " " + channel->getName() + " :" + channel->getTopic() + "\r\n");
        client->sendReply(":ircserv NOTICE " + client->getNickname() + " :Topic set by " +
                          channel->getTopicSetter() + " at " + Utils::formatTime(channel->getTopicTime()) + "\r\n");
    }
}

/**
 * @brief Validates and processes the topic setting per RFC 2812 §4.2.4.
 * Ensures exactly one topic parameter, strips leading ':', trims spaces, and enforces +t restriction.
 */
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

/**
 * @brief Main handler for the TOPIC command per RFC 2812 §4.2.4.
 * Coordinates validation, channel access, and topic viewing/setting logic.
 */
void handleTopic(std::list<std::string> cmdList, Client* client, Server* server) {
    if (!validateParameters(cmdList, client) || !validateClientRegistration(client)) {
        return;
    }

    std::list<std::string>::iterator it = cmdList.begin();
    ++it;
    std::string channelName = *it;

    Channel* channel = getChannel(server, channelName, client);
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