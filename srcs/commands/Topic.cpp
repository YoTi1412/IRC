#include "Command.hpp"

static bool validateJoinParameters(std::list<std::string>& cmdList, Client* client) {
    if (cmdList.size() < 2) {
        std::string nickname;
        if (client->getNickname().empty()) {
            nickname = "*";
        } else {
            nickname = client->getNickname();
        }
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_NEEDMOREPARAMS + " " +
                          nickname + " TOPIC :Not enough parameters");
        return false;
    }
    return true;
}

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

static Channel* getChannel(Server* server, const std::string& channelName, Client* client)
{
    std::map<std::string, Channel*>::iterator it = server->getChannels().find(channelName);
    if (it == server->getChannels().end())
    {
        std::ostringstream oss;
        oss << IRC_SERVER << " " << ERR_NOSUCHCHANNEL << " "
            << client->getNickname() << " " << channelName << " :No such channel";
        client->sendReply(oss.str());
        return NULL;
    }
    return it->second;
}

static bool checkExistingMembership(Channel* channel, Client* client) {
    if (!channel->isMember(client)) {
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_NOTONCHANNEL + " " +
                          client->getNickname() + " " + channel->getName() + " :You're not on that channel");
        return false;
    }
    return true;
}

std::string formatTime(time_t t)
{
    char buffer[64];
    struct tm *tm_info = localtime(&t);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
    return std::string(buffer);
}   


void broadcastTopicChange(Channel* channel, Client* client, const std::string& channelName, const std::string& topic)
{
    std::ostringstream oss;
    oss << ":" << client->getNickname() << "!" << client->getUsername()
        << "@" << client->getHostname()
        << " TOPIC " << channelName << " :" << topic;

    channel->broadcast(oss.str(), client);
}


void handleTopic(std::list<std::string> cmdList, Client* client, Server* server)
{
    if (!validateJoinParameters(cmdList, client) || !validateClientRegistration(client))
    {
        return;
    }

    std::list<std::string>::iterator it = cmdList.begin();
    ++it;
    std::string channelName = *it;

    Channel* channel = getChannel(server, channelName, client);
    if (!channel)
        return;
    if (!checkExistingMembership(channel, client))
    {
        return ;
    }
    ++it;
    if (it == cmdList.end())
    {
        if (channel->getTopic() == "")
        {
            client->sendReply(std::string(IRC_SERVER) + " " + RPL_NOTOPIC + " " +
                              client->getNickname() + " " + channel->getName() + " :No topic is set");
            return ;
        }
        else
        {
            client->sendReply(std::string(IRC_SERVER) + " " + RPL_TOPIC + " " +
                              client->getNickname() + " " + channel->getName() + " "  + channel->getTopic());
            std::string time;
            client->sendReply(std::string(IRC_SERVER) + " " + RPL_TOPICWHOTIME + " " +
                              client->getNickname() + " " + channel->getName() + " " + channel->getTopicSetter() + " " + formatTime(channel->getTopicTime()));
            return ;
        }
    }
    else
    {
        std::string topic = *it;
        if (!channel->getTopicRestricted())
        {
            channel->setTopic(topic, client);
            broadcastTopicChange(channel, client, channel->getName(), topic);
            return ;
        }
        else
        {
            if (!channel->isOperator(client))
            {
                client->sendReply(std::string(IRC_SERVER) + " " + ERR_CHANOPRIVSNEEDED + " " +
                              client->getNickname() + " " + channel->getName() + " :You're not channel operator");
                return ;
            }
            else
            {
                channel->setTopic(topic, client);
                broadcastTopicChange(channel, client, channel->getName(), topic);
                return ;
            }
        }
    }
}