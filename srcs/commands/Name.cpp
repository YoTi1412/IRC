#include "Command.hpp"
#include "Utils.hpp"
#include "Logger.hpp"
#include "Replies.hpp"
#include "Channel.hpp"

/**
 * @brief Validates client registration for NAMES command per RFC 2812 (Section 3.2.5).
 *
 * Parameters:
 * - client: Pointer to the client issuing the command.
 *
 * The function:
 * - Checks if the client is registered.
 * - Sends ERR_NOTREGISTERED (451) if not.
 *
 * @return true if registered, false otherwise.
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
 * @brief Retrieves the channel and validates access for NAMES per RFC 2812 (Section 3.2.5).
 *
 * Parameters:
 * - channelName: Name of the channel to list.
 * - client: Pointer to the client issuing the command.
 * - server: Pointer to the server instance.
 *
 * The function:
 * - Checks if the channel exists.
 * - Sends ERR_NOSUCHCHANNEL (403) if it doesn’t or if access is restricted.
 *
 * @return Pointer to the Channel object, or NULL if invalid.
 */
static Channel* getChannelForNames(const std::string& channelName, Client* client, Server* server) {
    std::map<std::string, Channel*>& channels = server->getChannels();
    std::map<std::string, Channel*>::iterator it = channels.find(channelName);
    if (it == channels.end()) {
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_NOSUCHCHANNEL + " " +
                          client->getNickname() + " " + channelName + " :No such channel\r\n");
        return NULL;
    }
    Channel* channel = it->second;
    if (!channel->isMember(client) && channel->getSecret()) { // Secret channels restrict non-members
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_NOSUCHCHANNEL + " " +
                          client->getNickname() + " " + channelName + " :No such channel\r\n");
        return NULL;
    }
    return channel;
}

/**
 * @brief Sends the NAMES response per RFC 2812 (Section 3.2.5).
 *
 * Parameters:
 * - channel: Pointer to the channel.
 * - client: Pointer to the client requesting the list.
 *
 * The function:
 * - Sends RPL_NAMREPLY (353) with the member list (including prefixes).
 * - Sends RPL_ENDOFNAMES (366) to conclude the response.
 */
static void sendNamesResponse(Channel* channel, Client* client) {
    std::string memberList = channel->getMemberList(); // Assume this returns @/+/nick format
    client->sendReply(std::string(IRC_SERVER) + " " + RPL_NAMREPLY + " " +
                      client->getNickname() + " = " + channel->getName() + " :" + memberList + "\r\n");
    client->sendReply(std::string(IRC_SERVER) + " " + RPL_ENDOFNAMES + " " +
                      client->getNickname() + " " + channel->getName() + " :End of NAMES list\r\n");
}

/**
 * @brief Handles the NAMES command per RFC 2812 (Section 3.2.5).
 *
 * Parameters:
 * - cmdList: List of command arguments (channel name, if provided).
 * - client: Pointer to the client issuing the command.
 * - server: Pointer to the server instance.
 *
 * The function:
 * - Validates client registration.
 * - Processes the request for a specific channel or all joined channels.
 */
void handleNames(std::list<std::string> cmdList, Client* client, Server* server) {
    if (!validateClientRegistration(client)) {
        return;
    }

    std::list<std::string>::iterator it = cmdList.begin();
    ++it; // Skip "NAMES"

    if (it == cmdList.end()) {
        std::map<std::string, Channel*>& channels = server->getChannels();
        for (std::map<std::string, Channel*>::iterator chanIt = channels.begin(); chanIt != channels.end(); ++chanIt) {
            if (chanIt->second->isMember(client)) {
                sendNamesResponse(chanIt->second, client);
            }
        }
    } else {
        std::string channelName = *it;
        Channel* channel = getChannelForNames(channelName, client, server);
        if (channel) {
            sendNamesResponse(channel, client);
        }
    }
}