#include "Command.hpp"
#include "Utils.hpp"
#include "Logger.hpp"
#include "Replies.hpp"
#include "Channel.hpp"

/**
 * @brief Verifies client registration status for NAMES command as per RFC 2812 (Section 3.2.5).
 */
static bool validateClientRegistration(Client* client) {
    if (!client->isRegistered()) {
        std::string nickname = client->getNickname().empty() ? "*" : client->getNickname();
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_NOTREGISTERED + " " +
                          nickname + " :You have not registered");
        return false;
    }
    return true;
}

/**
 * @brief Sends NAMES reply for a single channel as per RFC 2812 (Section 3.2.5).
 */
static void sendNamesReply(Channel* channel, Client* client) {
    if (channel == NULL) return;
    std::string memberList = channel->getMemberList();
    client->sendReply(std::string(IRC_SERVER) + " " + RPL_NAMREPLY + " " +
                      client->getNickname() + " = " + channel->getName() + " :" + memberList);
    client->sendReply(std::string(IRC_SERVER) + " " + RPL_ENDOFNAMES + " " +
                      client->getNickname() + " " + channel->getName() + " :End of NAMES list");
}

/**
 * @brief Collects all nicknames of users in channels as per RFC 2812 (Section 3.2.5).
 */
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

/**
 * @brief Collects stray users (registered, visible, not in any channel) as per RFC 2812 (Section 3.2.5).
 */
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

/**
 * @brief Sends NAMES replies for all visible channels and stray users as per RFC 2812 (Section 3.2.5).
 */
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
    }
    client->sendReply(std::string(IRC_SERVER) + " " + RPL_ENDOFNAMES + " " +
                      client->getNickname() + " * :End of NAMES list");
}

/**
 * @brief Handles NAMES command with no parameters as per RFC 2812 (Section 3.2.5).
 */
static void handleNamesNoParams(Server* server, Client* client) {
    sendAllNames(server, client);
}

/**
 * @brief Handles NAMES command with a list of channels as per RFC 2812 (Section 3.2.5).
 */
static void handleNamesWithChannels(std::list<std::string> channels, Client* client, Server* server) {
    std::map<std::string, Channel*>& chMap = server->getChannels();
    for (std::list<std::string>::iterator chanIt = channels.begin(); chanIt != channels.end(); ++chanIt) {
        std::map<std::string, Channel*>::iterator chIter = chMap.find(*chanIt);
        if (chIter != chMap.end()) {
            sendNamesReply(chIter->second, client);
        }
    }
}

/**
 * @brief Handles the NAMES command as per RFC 2812 (Section 3.2.5).
 */
void handleNames(std::list<std::string> cmdList, Client* client, Server* server) {
    if (!validateClientRegistration(client)) {
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

    std::list<std::string> channels = Utils::split(channelsStr, ',');
    handleNamesWithChannels(channels, client, server);
}
