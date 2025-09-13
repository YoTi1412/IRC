#include "Command.hpp"
#include "Utils.hpp"
#include "Logger.hpp"
#include "Replies.hpp"
#include "Channel.hpp"

/**
 * @brief Validates channel name according to RFC 2812 (Section 3.2.2).
 *
 * Parameters:
 * - name: The channel name to validate.
 *
 * The function:
 * - Checks if the name is non-empty, starts with '#', and is within 50 characters.
 * - Ensures the name does not contain spaces, commas, or ASCII 7 (bell).
 * - Verifies the name is not just '#' (must have at least one character after).
 *
 * @return true if the channel name is valid, false otherwise.
 */
static bool isValidChannelName(const std::string& name) {
    if (name.empty() || name.length() > 50 || name[0] != '#') {
        return false;
    }
    for (size_t i = 0; i < name.length(); ++i) {
        if (name[i] == ' ' || name[i] == ',' || name[i] == '\7') {
            return false;
        }
    }
    return name.length() > 1; // Ensure non-empty name after '#'
}

/**
 * @brief Checks if PART command has sufficient parameters as per RFC 2812 (Section 3.2.2).
 *
 * Parameters:
 * - cmdList: List of command arguments (expected to include channel names and optional message).
 * - client: Pointer to the client issuing the command.
 *
 * The function:
 * - Validates that at least one channel name is provided (minimum 2 elements in cmdList: "PART" and channels).
 * - Sends ERR_NEEDMOREPARAMS (461) reply if parameters are insufficient.
 *
 * @return true if parameters are valid, false otherwise.
 */
static bool validatePartParameters(std::list<std::string>& cmdList, Client* client) {
    if (cmdList.size() < 2) {
        std::string nickname;
        if (client->getNickname().empty()) {
            nickname = "*";
        } else {
            nickname = client->getNickname();
        }
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_NEEDMOREPARAMS + " " +
                          nickname + " PART :Not enough parameters");
        return false;
    }
    return true;
}

/**
 * @brief Verifies client registration status for PART command as per RFC 2812 (Section 3.2.2).
 *
 * Parameters:
 * - client: Pointer to the client issuing the command.
 *
 * The function:
 * - Checks if the client has completed registration.
 * - Sends ERR_NOTREGISTERED (451) reply if the client is not registered.
 *
 * @return true if the client is registered, false otherwise.
 */
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

/**
 * @brief Retrieves the channel if it exists as per RFC 2812 (Section 3.2.2).
 *
 * Parameters:
 * - channelName: Name of the channel to retrieve.
 * - client: Pointer to the client issuing the command.
 * - server: Pointer to the server instance managing channels.
 *
 * The function:
 * - Looks up the channel in the server's channel map.
 * - Sends ERR_NOSUCHCHANNEL (403) if the channel does not exist.
 *
 * @return Pointer to the Channel object if found, NULL otherwise.
 */
static Channel* getChannel(const std::string& channelName, Client* client, Server* server) {
    std::map<std::string, Channel*>& channels = server->getChannels();
    std::map<std::string, Channel*>::iterator it = channels.find(channelName);
    if (it == channels.end()) {
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_NOSUCHCHANNEL + " " +
                          client->getNickname() + " " + channelName + " :No such channel");
        return NULL;
    }
    return it->second;
}

/**
 * @brief Checks if the client is a member of the channel as per RFC 2812 (Section 3.2.2).
 *
 * Parameters:
 * - channel: Pointer to the channel being parted.
 * - client: Pointer to the client attempting to part.
 *
 * The function:
 * - Verifies if the client is a member of the specified channel.
 * - Sends ERR_NOTONCHANNEL (442) reply if the client is not on the channel.
 *
 * @return true if the client is a member, false otherwise.
 */
static bool checkMembership(Channel* channel, Client* client) {
    if (!channel->isMember(client)) {
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_NOTONCHANNEL + " " +
                          client->getNickname() + " " + channel->getName() + " :You're not on that channel");
        return false;
    }
    return true;
}

/**
 * @brief Broadcasts PART message to the channel as per RFC 2812 (Section 3.2.2).
 *
 * Parameters:
 * - channelName: Name of the channel being parted.
 * - message: The part message (default is the client's nickname if not provided).
 * - client: Pointer to the client parting the channel.
 * - channel: Pointer to the channel object.
 *
 * The function:
 * - Constructs and broadcasts the PART message to all channel members, including the sender.
 */
static void sendPartMessage(const std::string& channelName, const std::string& message, Client* client, Channel* channel) {
    std::string userIdent = client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname();
    std::string partMsg = ":" + userIdent + " PART " + channelName;
    if (!message.empty()) {
        partMsg += " :" + message;
    }
    channel->broadcast(partMsg, NULL);
}

/**
 * @brief Processes a single channel part operation as per RFC 2812 (Section 3.2.2).
 *
 * Parameters:
 * - channelName: Name of the channel to part.
 * - message: The part message to send.
 * - client: Pointer to the client attempting to part.
 * - server: Pointer to the server instance managing channels.
 *
 * The function:
 * - Validates the channel name.
 * - Retrieves the channel and checks membership.
 * - Broadcasts the PART message.
 * - Removes the client from the channel.
 * - Deletes the channel if it becomes empty.
 * - Logs the part event.
 */
static void processSinglePart(const std::string& channelName, const std::string& message, Client* client, Server* server) {
    if (!isValidChannelName(channelName)) {
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_BADCHANMASK + " " +
                          client->getNickname() + " " + channelName + " :Invalid channel name");
        return;
    }

    Channel* channel = getChannel(channelName, client, server);
    if (channel == NULL) {
        return;
    }

    if (!checkMembership(channel, client)) {
        return;
    }

    sendPartMessage(channelName, message, client, channel);

    channel->removeMember(client);

    if (channel->getMemberCount() == 0) {
        delete channel;
        server->getChannels().erase(channelName);
        Logger::info("Channel " + channelName + " deleted as it became empty");
    }

    Logger::info(client->getNickname() + " parted " + channelName);
}

/**
 * @brief Handles the PART command as per RFC 2812 (Section 3.2.2).
 *
 * Parameters:
 * - cmdList: List of command arguments (channel names and optional message).
 * - client: Pointer to the client issuing the command.
 * - server: Pointer to the server instance managing channels.
 *
 * The function:
 * - Validates parameters and client registration.
 * - Determines the part message (requires ':' prefix, defaults to nickname if not provided).
 * - Splits channel names using comma as delimiter.
 * - Processes each channel part individually.
 */
void handlePart(std::list<std::string> cmdList, Client* client, Server* server) {
    if (!validatePartParameters(cmdList, client) || !validateClientRegistration(client)) {
        return;
    }

    std::list<std::string>::iterator it = cmdList.begin();
    ++it; // Skip "PART"
    std::string channelsStr = *it;
    std::string message;
    if (++it != cmdList.end()) {
        std::string potentialMessage = *it;
        if (potentialMessage[0] == ':') {
            message = potentialMessage.substr(1);
            while (++it != cmdList.end()) {
                message += " " + *it; // Reconstruct message with spaces
            }
        } else {
            message = client->getNickname();
        }
    } else {
        message = client->getNickname();
    }

    std::list<std::string> channels = Utils::split(channelsStr, ',');
    for (std::list<std::string>::iterator chanIt = channels.begin(); chanIt != channels.end(); ++chanIt) {
        processSinglePart(*chanIt, message, client, server);
    }
}
