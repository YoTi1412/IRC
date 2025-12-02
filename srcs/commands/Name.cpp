#include "Includes.hpp"

static void sendNamesReply(Channel* channel, Client* client) {
    if (channel == NULL) return;
    std::string memberList = channel->getMemberList();
    client->sendReply(std::string(IRC_SERVER) + " " + RPL_NAMREPLY + " " +
                      client->getNickname() + " = " + channel->getName() + " :" + memberList);
    client->sendReply(std::string(IRC_SERVER) + " " + RPL_ENDOFNAMES + " " +
                      client->getNickname() + " " + channel->getName() + " :End of NAMES list");
}

static std::set<std::string> collectChannelMembers(Server* server) {
    std::set<std::string> inChannelNicks;
    std::map<std::string, Channel*>& channels = server->getChannels();

    for (std::map<std::string, Channel*>::iterator chanIt = channels.begin(); chanIt != channels.end(); ++chanIt) {
        std::string memberList = chanIt->second->getMemberList();
        std::list<std::string> members = Utils::split(memberList, ' ');
        for (std::list<std::string>::iterator memIt = members.begin(); memIt != members.end(); ++memIt) {
            std::string nick = *memIt;
            if (!nick.empty() && nick[0] == '@') {
                nick = nick.substr(1);
            }
            inChannelNicks.insert(nick);
        }
    }
    return inChannelNicks;
}

static std::string collectStrayUsers(Server* server, const std::set<std::string>& inChannelNicks) {
    std::string strayUsers;
    std::map<int, Client*>& clients = server->getClients();

    for (std::map<int, Client*>::iterator clIt = clients.begin(); clIt != clients.end(); ++clIt) {
        Client* c = clIt->second;
        if (c->isRegistered() && !c->getNickname().empty() &&
            inChannelNicks.find(c->getNickname()) == inChannelNicks.end()) {
            if (!strayUsers.empty()) strayUsers += " ";
            strayUsers += c->getNickname();
        }
    }
    return strayUsers;
}

static void sendAllNames(Server* server, Client* client) {
    std::map<std::string, Channel*>& channels = server->getChannels();
    for (std::map<std::string, Channel*>::iterator it = channels.begin(); it != channels.end(); ++it) {
        sendNamesReply(it->second, client);
    }

    std::set<std::string> inChannelNicks = collectChannelMembers(server);
    std::string strayUsers = collectStrayUsers(server, inChannelNicks);

    if (!strayUsers.empty()) {
        client->sendReply(std::string(IRC_SERVER) + " " + RPL_NAMREPLY + " " +
                          client->getNickname() + " = * :" + strayUsers);
        client->sendReply(std::string(IRC_SERVER) + " " + RPL_ENDOFNAMES + " " +
                          client->getNickname() + " * :End of NAMES list");
    }
}

static void handleNamesNoParams(Server* server, Client* client) {
    sendAllNames(server, client);
}

static void handleNamesWithChannels(std::list<std::string> channels, Client* client, Server* server) {
    std::map<std::string, Channel*>& chMap = server->getChannels();
    for (std::list<std::string>::iterator chanIt = channels.begin(); chanIt != channels.end(); ++chanIt) {
        std::string channelName = *chanIt;

        size_t start = channelName.find_first_not_of(" \t");
        size_t end = channelName.find_last_not_of(" \t");
        if (start == std::string::npos) {
            channelName = "";
        } else {
            channelName = channelName.substr(start, end - start + 1);
        }
        if (channelName.empty()) {
            handleNamesNoParams(server, client);
            continue;
        }
        std::map<std::string, Channel*>::iterator chIter = chMap.find(channelName);
        if (chIter != chMap.end()) {
            sendNamesReply(chIter->second, client);
        } else {
            client->sendReply(std::string(IRC_SERVER) + " " + RPL_ENDOFNAMES + " " +
                              client->getNickname() + " " + channelName + " :End of NAMES list");
        }
    }
}

void handleNames(std::list<std::string> cmdList, Client* client, Server* server) {
    if (!CommandUtils::validateClientRegistration(client)) {
        return;
    }
    std::list<std::string>::iterator it = cmdList.begin();
    ++it;
    if (it == cmdList.end()) {
        handleNamesNoParams(server, client);
        return;
    }
    std::string channelsStr = *it;
    ++it;
    if (channelsStr.empty()) {
        handleNamesNoParams(server, client);
        return;
    }
    std::list<std::string> channels = Utils::split(channelsStr, ',');
    handleNamesWithChannels(channels, client, server);
}
