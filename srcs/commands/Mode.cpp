
#include "Command.hpp"
#include "Utils.hpp"
#include "Logger.hpp"
#include "Replies.hpp"
#include "Channel.hpp"


bool checkChannelExist(Server* server, std::string channelName)
{
    std::map<std::string, Channel*>::iterator it = server->getChannels().find(channelName);

    if (it != server->getChannels().end()) {
        return true;
    }
    return false;
}

static bool validateCommand(std::list<std::string>& cmdList, Client* client)
{
    if (cmdList.size() < 3) {
        std::ostringstream oss;
        oss << IRC_SERVER << " " << ERR_NEEDMOREPARAMS << " " << client->getNickname() << " JOIN :Not enough parameters";
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

bool checkModes(std::string modes)
{
    std::string characters = "itkol";
    for (size_t i = 0; i < modes.length(); i++)
    {
        bool valid = false;
        for (size_t j = 0; j < characters.length(); j++)
        {
            if (modes[i] == characters[j])
            {
                valid = true;
                break;
            }
        }
        if (!valid)
            return false;
    }
    return true;
}


bool validateModes(std::string modes)
{
    if ((modes[0] == '+' || modes[0] == '-') && checkModes(modes.substr(1, modes.length())))
        return true;
    return false;
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

bool checkModesAndNotify(Client* client, const std::string& modes)
{
    if (!validateModes(modes))
    {
        std::ostringstream oss;
        oss << IRC_SERVER << " " << ERR_UNKNOWNMODE << " "
            << client->getNickname() << " " << modes
            << " :is unknown mode";
        client->sendReply(oss.str());
        return false;
    }
    return true;
}

void broadcastModeChange(Channel* channel, Client* client, const std::string& channelName, const std::string& modes)
{
    std::ostringstream oss;
    oss << ":" << client->getNickname() << "!" << client->getUsername()
        << "@" << client->getHostname()
        << " MODE " << channelName << " " << modes;

    channel->broadcast(oss.str(), client);
}



void sendError(Client* client, const std::string& code, const std::string& message)
{
    std::ostringstream oss;
    oss << IRC_SERVER << " " << code << " " << client->getNickname() << " " << message;
    client->sendReply(oss.str());
}

bool processOperatorMode(char sign, std::list<std::string>::iterator& it, std::list<std::string>::iterator end, Client* sender, Server* server, Channel* channel)
{
    if (it == end)
    {
        sendError(sender, ERR_NEEDMOREPARAMS, "MODE :Not enough parameters");
        return false;
    }
    std::string targetNick = *it;
    ++it;
    Client* target = server->getClientByNickname(targetNick);
    if (!target || !channel->isMember(target))
    {
        std::ostringstream oss;
        oss << IRC_SERVER << " " << ERR_USERNOTINCHANNEL << " "
            << sender->getNickname() << " " << targetNick << " "
            << channel->getName() << " :They aren't on that channel";
        sender->sendReply(oss.str());
        return false;
    }
    if (sign == '+')
        channel->addOperator(target->getFd());
    else
        channel->removeOperator(target->getFd());
    std::ostringstream oss;
    oss << ":" << sender->getNickname() << "!" << sender->getUsername()
        << "@" << sender->getHostname()
        << " MODE " << channel->getName() << " " << sign << "o " << targetNick;
    channel->broadcast(oss.str(), sender);
    return true;
}

bool processSimpleMode(char sign, char modeChar, Client* sender)
{
    std::string modeStr = std::string(1, sign) + std::string(1, modeChar);
    return checkModesAndNotify(sender, modeStr);
}


void handleMode(std::list<std::string> cmdList, Client* client, Server* server)
{
    if (!validateCommand(cmdList, client))
        return;
    std::list<std::string>::iterator it = cmdList.begin();
    ++it;
    std::string channelName = *it;

    Channel* channel = getChannel(server, channelName, client);
    if (!channel)
        return;
    ++it;
    if (it == cmdList.end())
        return;
    std::string modes = *it;
    ++it;
    char sign = modes[0];
    for (size_t i = 1; i < modes.size(); ++i)
    {
        char modeChar = modes[i];

        if (modeChar == 'o')
        {
            if (!processOperatorMode(sign, it, cmdList.end(), client, server, channel))
                return;
        }
        else
        {
            if (!processSimpleMode(sign, modeChar, client))
                return;
        }
    }
}
