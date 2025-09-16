
#include "Command.hpp"
#include "Utils.hpp"
#include "Logger.hpp"
#include "Replies.hpp"
#include "Channel.hpp"


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


static bool validateCommand(std::list<std::string>& cmdList, Client* client)
{
    if (cmdList.size() < 3) {
        std::ostringstream oss;
        oss << IRC_SERVER << " " << ERR_NEEDMOREPARAMS << " " << client->getNickname() << " MODE :Not enough parameters";
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

bool checkChannelOperator(Client* client, Channel* channel)
{
    if (!channel->isOperator(client))
    {
        sendError(client, ERR_CHANOPRIVSNEEDED, channel->getName() + " :You're not channel operator");
        return false;
    }
    return true;
}

std::vector<std::string> splitMode(std::string modes)
{
    std::vector<std::string> mode;
    size_t i = 0;
    while(i < modes.length())
    {
        if (modes[i] == '+')
        {
            i++;
            while(i < modes.length() && modes[i] != '+' && modes[i] != '-')
            {
                std::string md = std::string("+") + modes[i];
                mode.push_back(md);
                i++;
            }
        }
        else if (modes[i] == '-')
        {
            i++;
            while(i < modes.length() && modes[i] != '+' && modes[i] != '-')
            {
                std::string md = std::string("-") + modes[i];
                mode.push_back(md);
                i++;
            }
        }
        else
            break;
    }
    return mode;
}

bool isParameter(std::string modes)
{
    if (modes[0] == '+' || modes[0] == '-')
        return false;
    else
        return true;
}

bool needsParameter(std::string mode)
{
    if (mode == "+i" || mode == "-i")
        return false;
    else if (mode == "+k")
        return true;
    else if (mode == "-k")
        return false;
    else if (mode == "+o" || mode == "-o")
        return true;
    else if (mode == "+l")
        return true;
    else if (mode == "-l")
        return false;
    return false;
}


void handleNonParameter(std::string mode, Channel* channel, Client* client)
{
    if (mode == "+i")
    {
        channel->setInviteOnly(true);
        broadcastModeChange(channel, client, channel->getName(), mode);
    }
    else if (mode == "-i")
    {
        channel->setInviteOnly(false);
        broadcastModeChange(channel, client, channel->getName(), mode);
    }
    else if (mode == "+t")
    {
        channel->setTopicRestricted(true);
        broadcastModeChange(channel, client, channel->getName(), mode);
    }
    else if (mode == "-t")
    {
        channel->setTopicRestricted(false);
        broadcastModeChange(channel, client, channel->getName(), mode);
    }
    else if (mode == "-k")
    {
        channel-> setKey("");
        broadcastModeChange(channel, client, channel->getName(), mode);
    }
    else if (mode == "-l")
    {
        channel->setLimited(false);
        broadcastModeChange(channel, client, channel->getName(), mode);
    }
}


void handleWithParameter(std::string mode, std::string parameter, Channel* channel, Client* client, Server* server)
{
    if (mode == "+k")
    {
        channel->setKey(parameter);
        broadcastModeChange(channel, client, channel->getName(), mode);
    }
    else if (mode == "+l")
    {
        channel->setLimited(true);
        channel->setLimit(std::atoi(parameter.c_str()));
        broadcastModeChange(channel, client, channel->getName(), mode);
    }
    else if (mode == "+o")
    {
        Client* targetClient = server->getClientByNickname(parameter);
        if (!targetClient || !channel->isMember(targetClient))
        {
            client->sendReply(std::string(IRC_SERVER) + " " + ERR_USERNOTINCHANNEL + " " +
                            client->getNickname() + " " + parameter + " " + channel->getName() +
                            " :They aren't on that channel");
            return;
        }
        channel->addOperator(targetClient->getFd());
        broadcastModeChange(channel, client, channel->getName(), mode);
    }
    else if (mode == "-o")
    {
        Client* targetClient = server->getClientByNickname(parameter);
        if (!targetClient || !channel->isMember(targetClient))
        {
            client->sendReply(std::string(IRC_SERVER) + " " + ERR_USERNOTINCHANNEL + " " +
                            client->getNickname() + " " + parameter + " " + channel->getName() +
                            " :They aren't on that channel");
            return;
        }
        channel->removeOperator(targetClient->getFd());
        broadcastModeChange(channel, client, channel->getName(), mode);
    }

}


void handleTheMode(std::vector<std::string> modesVector, std::vector<std::string> ParametersVector, Client* client, Channel* channel, Server* server)
{
    size_t paramIndex = 0;
    for (size_t i = 0; i < modesVector.size(); i++)
    {
        if (needsParameter(modesVector[i]))
        {
            if (paramIndex >= ParametersVector.size())
            {
                client->sendReply(std::string(IRC_SERVER) + " " + ERR_NEEDMOREPARAMS + " " +
                          client->getNickname() + " MODE :Not enough parameters");
                return;
            }
            handleWithParameter(modesVector[i], ParametersVector[paramIndex], channel, client, server);
            std::cout << paramIndex++;
        }
        else
        {
            handleNonParameter(modesVector[i], channel, client);
        }
    }
}

static bool validateClientRegistration(Client* client)
{
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


void handleMode(std::list<std::string> cmdList, Client* client, Server* server)
{
    std::vector<std::string> modesVector;
    std::vector<std::string> ParametersVector;
    if (!validateClientRegistration(client) || !validateCommand(cmdList, client))
        return;
    std::list<std::string>::iterator it = cmdList.begin();
    ++it;
    std::string channelName = *it;

    Channel* channel = getChannel(server, channelName, client);
    if (!channel)
        return;
    if (!checkChannelOperator(client, channel))
        return;
    ++it;
    if (it == cmdList.end())
        return;
    for (; it != cmdList.end(); ++it)
    {
        std::string current = *it;
        if (isParameter(current))
            ParametersVector.push_back(current);
        else
        {
            std::vector<std::string> mode = splitMode(current);
            for (size_t i = 0; i < mode.size(); i++)
            {
                modesVector.push_back(mode[i]);
            }
        }
    }
    handleTheMode(modesVector, ParametersVector, client, channel, server);
}
