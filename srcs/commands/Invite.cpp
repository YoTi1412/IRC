#include "Includes.hpp"

static bool validateCommand(std::list<std::string>& cmdList, Client* client)
{
    if (!CommandUtils::validateParameters(cmdList, client, "INVITE", 3))
        return false;
    if (!CommandUtils::validateClientRegistration(client))
        return false;
    return true;
}

static bool validateInviteCommand(std::list<std::string>& cmdList, Client* client)
{
    if (!validateCommand(cmdList, client))
        return false;
    return true;
}

static bool validateChannelAndPermissions(Channel* channel, Client* client)
{
    if (!channel->isMember(client))
    {
        std::ostringstream oss;
        oss << IRC_SERVER << " " << ERR_NOTONCHANNEL << " "
            << client->getNickname() << " " << channel->getName() << " :You're not on that channel";
        client->sendReply(oss.str());
        return false;
    }

    if (channel->getInviteOnly() && !channel->isOperator(client))
    {
        std::ostringstream oss;
        oss << IRC_SERVER << " " << ERR_CHANOPRIVSNEEDED << " "
            << client->getNickname() << " " << channel->getName() << " :You're not channel operator";
        client->sendReply(oss.str());
        return false;
    }
    return true;
}

static bool checkTargetNotInChannel(Channel* channel, Client* sender, Client* target)
{
    if (channel->isMember(target))
    {
        std::ostringstream oss;
        oss << IRC_SERVER << " " << ERR_USERONCHANNEL << " "
            << sender->getNickname() << " " << target->getNickname() << " "
            << channel->getName() << " :is already on channel";
        sender->sendReply(oss.str());
        return false;
    }
    return true;
}

static void sendInvite(Channel* channel, Client* sender, Client* target)
{
    channel->addInvite(target->getFd());
    std::ostringstream oss;
    oss << ":" << sender->getNickname() << "!" << sender->getUsername()
        << "@" << sender->getHostname()
        << " INVITE " << target->getNickname() << " " << channel->getName();
    target->sendReply(oss.str());
    sender->sendReply(oss.str());
    Logger::info(sender->getNickname() + " invited " + target->getNickname() + " to " + channel->getName());
}

void handleInvite(std::list<std::string> cmdList, Client* client, Server* server)
{
    if (!validateInviteCommand(cmdList, client))
        return;

    std::list<std::string>::iterator it = cmdList.begin();
    ++it;
    std::string channelName = *it;
    Channel* channel = CommandUtils::getChannel(server, channelName, client);
    if (!channel || !validateChannelAndPermissions(channel, client))
        return;

    ++it;
    std::string targetNick = *it;
    Client* target = CommandUtils::getTargetClient(server, client, targetNick);
    if (!target || !checkTargetNotInChannel(channel, client, target))
        return;

    sendInvite(channel, client, target);
}
