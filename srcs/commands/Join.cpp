#include "Command.hpp"
#include "Utils.hpp"
#include "Logger.hpp"
#include "Replies.hpp"
#include "Channel.hpp"

/**
 * @brief Validates channel name according to RFC 2812 (Section 3.2.1).
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
    if (name.empty() || name.length() > 50 || name[0] != '#') { // Max 50 chars per RFC 2812
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
 * @brief Checks if JOIN command has sufficient parameters as per RFC 2812 (Section 3.2.1).
 *
 * Parameters:
 * - cmdList: List of command arguments (expected to include channel names and optional keys).
 * - client: Pointer to the client issuing the command.
 *
 * The function:
 * - Validates that at least one channel name is provided (minimum 2 elements in cmdList: "JOIN" and channels).
 * - Sends ERR_NEEDMOREPARAMS (461) reply if parameters are insufficient.
 *
 * @return true if parameters are valid, false otherwise.
 */
static bool validateJoinParameters(std::list<std::string>& cmdList, Client* client) {
    if (cmdList.size() < 2) {
        std::string nickname;
        if (client->getNickname().empty()) {
            nickname = "*";
        } else {
            nickname = client->getNickname();
        }
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_NEEDMOREPARAMS + " " +
                          nickname + " JOIN :Not enough parameters");
        return false;
    }
    return true;
}

/**
 * @brief Verifies client registration status for JOIN command as per RFC 2812 (Section 3.2.1).
 *
 * Parameters:
 * - client: Pointer to the client issuing the command.
 *
 * The function:
 * - Checks if the client has completed registration (e.g., NICK and USER commands processed).
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
 * @brief Creates or retrieves a channel as per RFC 2812 (Section 3.2.1).
 *
 * Parameters:
 * - channelName: Name of the channel to join or create.
 * - client: Pointer to the client issuing the command.
 * - server: Pointer to the server instance managing channels.
 *
 * The function:
 * - Checks if the channel exists in the server's channel map.
 * - Creates a new channel with the client as the operator if it does not exist.
 * - Retrieves the existing channel if it already exists.
 *
 * @return Pointer to the Channel object (newly created or existing).
 */
 static Channel* getOrCreateChannel(const std::string& channelName, Client* client, Server* server) {
     std::map<std::string, Channel*>& channels = server->getChannels();
     Channel* channel = NULL;
     std::map<std::string, Channel*>::iterator it = channels.find(channelName);
     if (it == channels.end()) {
         channel = new Channel(channelName, client);
         channels[channelName] = channel;
     } else {
         channel = it->second;
     }
     return channel;
 }

/**
 * @brief Checks if a client is already a member of a channel as per RFC 2812 (Section 3.2.1).
 *
 * Parameters:
 * - channel: Pointer to the channel being joined.
 * - client: Pointer to the client attempting to join.
 *
 * The function:
 * - Verifies if the client is already a member of the specified channel.
 * - Sends ERR_USERONCHANNEL (443) reply if the client is already a member.
 *
 * @return true if the client is already a member, false otherwise.
 */
static bool checkExistingMembership(Channel* channel, Client* client) {
    if (channel->isMember(client)) {
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_USERONCHANNEL + " " +
                          client->getNickname() + " " + channel->getName() + " :is already on channel");
        return true;
    }
    return false;
}

/**
 * @brief Broadcasts JOIN message and sends topic/replies as per RFC 2812 (Section 3.2.1).
 *
 * Parameters:
 * - channelName: Name of the channel being joined.
 * - channel: Pointer to the channel object.
 * - client: Pointer to the client joining the channel.
 *
 * The function:
 * - Broadcasts the JOIN message to all channel members.
 * - Sends RPL_TOPIC (332) if the channel has a topic.
 * - Sends RPL_NAMREPLY (353) and RPL_ENDOFNAMES (366) to the joining client.
 */
static void sendJoinMessages(const std::string& channelName, Channel* channel, Client* client) {
    std::string userIdent = client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname();
    std::string joinMsg = ":" + userIdent + " JOIN " + channelName;
    channel->broadcast(joinMsg, NULL);

    if (!channel->getTopic().empty()) {
        client->sendReply(std::string(IRC_SERVER) + " " + RPL_TOPIC + " " +
                          client->getNickname() + " " + channelName + " :" + channel->getTopic());
    }

    client->sendReply(std::string(IRC_SERVER) + " " + RPL_NAMREPLY + " " +
                      client->getNickname() + " = " + channelName + " :" + channel->getMemberList());
    client->sendReply(std::string(IRC_SERVER) + " " + RPL_ENDOFNAMES + " " +
                      client->getNickname() + " " + channelName + " :End of NAMES list");
}

/**
 * @brief Processes a single channel join operation as per RFC 2812 (Section 3.2.1).
 *
 * Parameters:
 * - channelName: Name of the channel to join.
 * - key: Channel key (currently unused, reserved for future implementation).
 * - client: Pointer to the client attempting to join.
 * - server: Pointer to the server instance managing channels.
 *
 * The function:
 * - Validates the channel name.
 * - Retrieves or creates the channel.
 * - Checks for existing membership.
 * - Adds the client to the channel and sends appropriate messages.
 * - Logs the join event.
 */
 static void processSingleJoin(const std::string& channelName, const std::string& key, Client* client, Server* server) {
     (void)key;
     if (!isValidChannelName(channelName)) {
         client->sendReply(std::string(IRC_SERVER) + " " + ERR_BADCHANMASK + " " +
                           client->getNickname() + " " + channelName + " :Invalid channel name");
         return;
     }

     Channel* channel = getOrCreateChannel(channelName, client, server);
     if (checkExistingMembership(channel, client)) {
         return;
     }

     channel->addMember(client);
     sendJoinMessages(channelName, channel, client);
     Logger::info(client->getNickname() + " joined " + channelName);
 }
/**
 * @brief Handles the JOIN command as per RFC 2812 (Section 3.2.1).
 *
 * Parameters:
 * - cmdList: List of command arguments (channel names and optional keys).
 * - client: Pointer to the client issuing the command.
 * - server: Pointer to the server instance managing channels.
 *
 * The function:
 * - Validates parameters and client registration.
 * - Splits channel names and keys (if provided) using comma as delimiter.
 * - Processes each channel join individually.
 */
void handleJoin(std::list<std::string> cmdList, Client* client, Server* server) {
    if (!validateJoinParameters(cmdList, client) || !validateClientRegistration(client)) {
        return;
    }

    std::list<std::string>::iterator it = cmdList.begin();
    ++it; // Skip "JOIN"
    std::list<std::string> channels = Utils::split(*it, ',');
    std::list<std::string> keys;
    if (++it != cmdList.end()) {
        keys = Utils::split(*it, ',');
    }

    std::list<std::string>::iterator keyIt = keys.begin();
    for (std::list<std::string>::iterator chanIt = channels.begin(); chanIt != channels.end(); ++chanIt) {
        std::string key = (keyIt != keys.end()) ? *keyIt++ : "";
        processSingleJoin(*chanIt, key, client, server);
    }
}
