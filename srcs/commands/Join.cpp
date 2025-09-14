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
        std::string nickname = client->getNickname().empty() ? "*" : client->getNickname();
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_NEEDMOREPARAMS + " " +
                          nickname + " JOIN :Not enough parameters\r\n");
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
        std::string nickname = client->getNickname().empty() ? "*" : client->getNickname();
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_NOTREGISTERED + " " +
                          nickname + " :You have not registered\r\n");
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
 * - Creates a new channel if it does not exist, setting the creator as the first operator.
 *
 * @return Pointer to the Channel object.
 */
static Channel* getOrCreateChannel(const std::string& channelName, Client* client, Server* server) {
    std::map<std::string, Channel*>& channels = server->getChannels();
    std::map<std::string, Channel*>::iterator chanIt = channels.find(channelName);
    if (chanIt == channels.end()) {
        Channel* newChannel = new Channel(channelName, client);
        channels[channelName] = newChannel;
        newChannel->addOperator(client->getFd()); // First member is operator
        Logger::info("Channel " + channelName + " created by " + client->getNickname() + ", " +
                     client->getNickname() + " set as operator");
        return newChannel;
    }
    return chanIt->second;
}

/**
 * @brief Checks if the client is already a member of the channel as per RFC 2812 (Section 3.2.1).
 *
 * Parameters:
 * - channel: Pointer to the channel.
 * - client: Pointer to the client attempting to join.
 *
 * The function:
 * - Checks membership using Channel::isMember.
 * - Sends ERR_USERONCHANNEL (443) if the client is already a member.
 *
 * @return true if the client is already a member, false otherwise.
 */
static bool checkExistingMembership(Channel* channel, Client* client) {
    if (channel->isMember(client)) {
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_USERONCHANNEL + " " +
                          client->getNickname() + " " + channel->getName() + " :You are already on that channel\r\n");
        return true;
    }
    return false;
}

/**
 * @brief Validates channel mode restrictions as per RFC 2812 (Section 3.2.1).
 *
 * Parameters:
 * - channel: Pointer to the channel.
 * - client: Pointer to the client attempting to join.
 * - key: The channel key provided by the client.
 *
 * The function:
 * - Checks for +i (invite-only), +k (key-protected), and +l (limit) modes.
 * - Sends appropriate error replies (e.g., ERR_INVITEONLYCHAN, ERR_BADCHANNELKEY, ERR_CHANNELISFULL).
 *
 * @return true if all mode checks pass, false otherwise.
 */
static bool validateChannelModes(Channel* channel, Client* client, const std::string& key) {
    if (channel->getInviteOnly() && !channel->isInvited(client->getFd())) {
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_INVITEONLYCHAN + " " +
                          client->getNickname() + " " + channel->getName() + " :Cannot join channel (+i)\r\n");
        Logger::warning(client->getNickname() + " failed to join " + channel->getName() + " due to +i restriction");
        return false;
    }
    if (channel->getKeyProtected()) {
        if (key.empty()) {
            client->sendReply(std::string(IRC_SERVER) + " " + ERR_BADCHANNELKEY + " " +
                              client->getNickname() + " " + channel->getName() + " :Key required (+k)\r\n");
            Logger::warning(client->getNickname() + " failed to join " + channel->getName() + " due to missing key");
            return false;
        }
        if (key != channel->getKey()) {
            client->sendReply(std::string(IRC_SERVER) + " " + ERR_BADCHANNELKEY + " " +
                              client->getNickname() + " " + channel->getName() + " :Incorrect key (+k)\r\n");
            Logger::warning(client->getNickname() + " failed to join " + channel->getName() + " due to wrong key");
            return false;
        }
    }
    if (channel->getLimited()) {
        if (channel->getLimit() == 0) {
            Logger::warning("Channel " + channel->getName() + " has limit 0, join denied");
            client->sendReply(std::string(IRC_SERVER) + " " + ERR_CHANNELISFULL + " " +
                              client->getNickname() + " " + channel->getName() + " :Channel limit is 0 (+l)\r\n");
            return false;
        }
        if (channel->getMemberCount() >= channel->getLimit()) {
            client->sendReply(std::string(IRC_SERVER) + " " + ERR_CHANNELISFULL + " " +
                              client->getNickname() + " " + channel->getName() + " :Cannot join channel (+l)\r\n");
            Logger::warning(client->getNickname() + " failed to join " + channel->getName() + " due to +l limit");
            return false;
        }
    }
    return true;
}

/**
 * @brief Sends JOIN-related messages to the client as per RFC 2812 (Section 3.2.1).
 *
 * Parameters:
 * - channelName: Name of the channel joined.
 * - channel: Pointer to the channel.
 * - client: Pointer to the client who joined.
 *
 * The function:
 * - Broadcasts the JOIN message to all channel members.
 * - Sends RPL_TOPIC (332) if a topic is set, with NOTICE for setter/time.
 * - Sends RPL_NAMREPLY (353) with member list and RPL_ENDOFNAMES (366).
 */
static void sendJoinMessages(const std::string& channelName, Channel* channel, Client* client) {
    std::string userIdent = client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname();
    std::string joinMsg = ":" + userIdent + " JOIN " + channelName + "\r\n";
    channel->broadcast(joinMsg, NULL);

    if (!channel->getTopic().empty()) {
        client->sendReply(std::string(IRC_SERVER) + " " + RPL_TOPIC + " " +
                          client->getNickname() + " " + channelName + " :" + channel->getTopic() + "\r\n");
        client->sendReply(":ircserv NOTICE " + client->getNickname() + " :Topic set by " +
                          channel->getTopicSetter() + " at " + Utils::formatTime(channel->getTopicTime()) + "\r\n");
    }

    client->sendReply(std::string(IRC_SERVER) + " " + RPL_NAMREPLY + " " +
                      client->getNickname() + " = " + channelName + " :" + channel->getMemberList() + "\r\n");
    client->sendReply(std::string(IRC_SERVER) + " " + RPL_ENDOFNAMES + " " +
                      client->getNickname() + " " + channelName + " :End of NAMES list\r\n");
}

/**
 * @brief Processes a single channel join operation as per RFC 2812 (Section 3.2.1).
 *
 * Parameters:
 * - channelName: Name of the channel to join.
 * - key: Channel key provided by the client.
 * - client: Pointer to the client attempting to join.
 * - server: Pointer to the server instance managing channels.
 *
 * The function:
 * - Validates the channel name.
 * - Retrieves or creates the channel.
 * - Checks for existing membership and mode restrictions.
 * - Adds the client to the channel and sends appropriate messages.
 * - Logs the join event.
 */
static void processSingleJoin(const std::string& channelName, const std::string& key, Client* client, Server* server) {
    if (!isValidChannelName(channelName)) {
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_BADCHANMASK + " " +
                          client->getNickname() + " " + channelName + " :Invalid channel name\r\n");
        return;
    }

    Channel* channel = getOrCreateChannel(channelName, client, server);
    if (checkExistingMembership(channel, client)) {
        return;
    }

    if (!validateChannelModes(channel, client, key)) {
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