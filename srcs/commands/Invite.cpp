#include "Channel.hpp"
#include "Replies.hpp"
#include "Command.hpp"
#include "Utils.hpp"
#include "Logger.hpp"


static bool validateCommand(std::list<std::string>& cmdList, Client* client)
{
    if (cmdList.size() < 3) {
        std::ostringstream oss;
        oss << IRC_SERVER << " " << ERR_NEEDMOREPARAMS << " " << client->getNickname() << " INVITE :Not enough parameters";
        client->sendReply(oss.str());
        return false;
    }
    if (!client->isRegistered()) {
        std::ostringstream oss;
        oss << IRC_SERVER << " " << ERR_NOTREGISTERED << " " << client->getNickname() << " :You have not registered";
        client->sendReply(oss.str());
        return false;
    }
    return true;
}

Channel* getChannel(Server* server, const std::string& channelName, Client* client)
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

static Client* getTargetClient(Server* server, Client* sender, const std::string& targetNick)
{
    Client* target = server->getClientByNickname(targetNick);
    if (!target)
    {
        std::ostringstream oss;
        oss << IRC_SERVER << " " << ERR_NOSUCHNICK << " "
            << sender->getNickname() << " " << targetNick << " :No such nickname";
        sender->sendReply(oss.str());
        return NULL;
    }
    return target;
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

void handleInvite(std::list<std::string> cmdList, Client* client, Server* server)
{
    if (!validateInviteCommand(cmdList, client) || validateClientRegistration(client))
        return;

    std::list<std::string>::iterator it = cmdList.begin();
    ++it;
    std::string channelName = *it;
    Channel* channel = getChannel(server, channelName, client);
    if (!channel || !validateChannelAndPermissions(channel, client))
        return;

    ++it;
    std::string targetNick = *it;
    Client* target = getTargetClient(server, client, targetNick);
    if (!target || !checkTargetNotInChannel(channel, client, target))
        return;

    sendInvite(channel, client, target);
}
