#include "Includes.hpp"

static bool validateJoinParameters(std::list<std::string>& cmdList, Client* client) {
    return CommandUtils::validateParameters(cmdList, client, "KICK", 2);
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
        << " KICK " << channelName << " " << target  << " :";

    channel->broadcast(oss.str(), client);
}

void broadcastKickWithComment(Channel* channel, Client* client, const std::string& channelName, const std::string& target, const std::string& comment)
{
    std::ostringstream oss;
    oss << ":" << client->getNickname() << "!" << client->getUsername()
        << "@" << client->getHostname()
        << " KICK " << channelName << " " << target  << " :" << comment;

    channel->broadcast(oss.str(), client);
}

void handleKick(std::list<std::string> cmdList, Client* client, Server* server)
{
    if (!validateJoinParameters(cmdList, client) || !CommandUtils::validateClientRegistration(client))
    {
        return;
    }
    std::list<std::string>::iterator it = cmdList.begin();
    ++it;
    std::string channelName = *it;

    Channel* channel = CommandUtils::getChannel(server, channelName, client);
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
            server->removeChannel(channelName);
        }
    }
    for (std::map<std::string, Channel*>::iterator it = server->getChannels().begin(); it != server->getChannels().end(); ++it) {
        std::cout << it->first << std::endl;
    }

}
