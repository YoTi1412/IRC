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

static bool checkExistingMembership(Channel* channel, Client* client)
{
    if (!channel->isMember(client))
    {
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_NOTONCHANNEL + " " +
                          client->getNickname() + " " + channel->getName() + " :You're not on that channel");
        return false;
    }
    if (!channel->isOperator(client))
    {
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_CHANOPRIVSNEEDED + " " +
                              client->getNickname() + " " + channel->getName() + " :You're not channel operator");
        return false;
    }
    return true;
}

void broadcastKickNoComment(Channel* channel, Client* client, const std::string& channelName, const std::string& target)
{
    std::ostringstream oss;
    oss << ":" << client->getNickname() << "!" << client->getUsername()
        << "@" << client->getHostname()
        << " KICK " << channelName << target  << " :";

    channel->broadcast(oss.str(), client);
}

void broadcastKickWithComment(Channel* channel, Client* client, const std::string& channelName, const std::string& target, const std::string& comment)
{
    std::ostringstream oss;
    oss << ":" << client->getNickname() << "!" << client->getUsername()
        << "@" << client->getHostname()
        << " KICK " << channelName << target  << " :" << comment;

    channel->broadcast(oss.str(), client);
}

void handleKick(std::list<std::string> cmdList, Client* client, Server* server)
{
    if (!validateJoinParameters(cmdList, client) || !validateClientRegistration(client))
    {
        return;
    }
    std::list<std::string>::iterator it = cmdList.begin();
    ++it;
    std::string channelName = *it;

    Channel* channel = getChannel(server, channelName, client);
    if (!channel || !checkExistingMembership(channel, client))
        return;
    ++it;
    std::string target = *it;
    Client* targetClient = server->getClientByNickname(target);
    if (!targetClient || !channel->isMember(targetClient))
    {
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_USERNOTINCHANNEL + " " +
                        client->getNickname() + " " + target + " " + channel->getName() +
                        " :They aren't on that channel");
        return;
    }
    else
    {
        ++it;
        if (it == cmdList.end())
            broadcastKickNoComment(channel, client, channel->getName(), target);
        else
        {
            std::string comment = *it;
            broadcastKickWithComment(channel, client, channel->getName(), target, comment);
        }
        channel->removeMember(targetClient);
        if (channel->getMemberCount() == 0)
        {
            server->removeChannel(channel->getName());
        }
    }
    for (std::map<std::string, Channel*>::iterator it = server->getChannels().begin(); it != server->getChannels().end(); ++it) {
        std::cout << it->first << std::endl;
    }

}