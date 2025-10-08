#include "Command.hpp"
#include "Utils.hpp"
#include "Logger.hpp"
#include "Replies.hpp"
#include <sstream>

/**
 * @brief Validates client registration for PRIVMSG command as per RFC 2812 (Section 3.3.1).
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


/**
 * @brief Checks if PRIVMSG command has sufficient parameters as per RFC 2812 (Section 3.3.1).
 *
 * Parameters:
 * - cmdList: List of command arguments (target and message).
 * - client: Pointer to the client issuing the command.
 *
 * The function:
 * - Ensures at least one target is provided (minimum 2 elements in cmdList: "PRIVMSG" and target).
 * - Ensures a message is provided (minimum 3 elements in cmdList).
 * - Sends ERR_NORECIPIENT (411) if no target is provided.
 * - Sends ERR_NOTEXTTOSEND (412) if no message is provided.
 *
 * @return true if parameters are valid, false otherwise.
 */
static bool validatePrivmsgParameters(std::list<std::string>& cmdList, Client* client) {
    if (!CommandUtils::validateParameters(cmdList, client, "PRIVMSG", 2)) {
        return false;
    }
    if (cmdList.size() < 3) {
        std::string nickname = CommandUtils::getNicknameOrDefault(client, "*");
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_NOTEXTTOSEND + " " +
                          nickname + " :No text to send");
        return false;
    }
    return true;
}

/**
 * @brief Validates message length for PRIVMSG as per RFC 2812 (Section 3.3.1).
 *
 * Parameters:
 * - message: The message text to validate.
 * - client: Pointer to the client issuing the command.
 *
 * The function:
 * - Checks if the message length (including CRLF) exceeds MAX_MESSAGE_LENGTH.
 * - Sends ERR_TOOMANYCHANNELS (405) reply if the message is too long (reused for length error).
 *
 * @return true if the message length is valid, false otherwise.
 */
static bool validateMessageLength(const std::string& message, Client* client) {
    std::string fullMsgWithCRLF = message + "\r\n";
    if (fullMsgWithCRLF.length() > MAX_MESSAGE_LENGTH) {
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_TOOMANYCHANNELS + " " +
                          client->getNickname() + " :Message too long");
        return false;
    }
    return true;
}

/**
 * @brief Constructs the full PRIVMSG message with user identifier as per RFC 2812 (Section 3.3.1).
 *
 * Parameters:
 * - targetsStr: The target string (comma-separated list of recipients).
 * - message: The message text to send.
 * - client: Pointer to the client issuing the command.
 *
 * The function:
 * - Builds the message in the format ":nick!user@host PRIVMSG targets :message".
 *
 * @return The constructed PRIVMSG message string.
 */
static std::string constructFullMessage(const std::string& targetsStr, const std::string& message, Client* client) {
    std::string userIdent = client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname();
    return ":" + userIdent + " PRIVMSG " + targetsStr + " :" + message;
}

/**
 * @brief Processes a PRIVMSG to a channel as per RFC 2812 (Section 3.3.1).
 *
 * Parameters:
 * - target: The channel name (starts with '#').
 * - fullMsg: The complete PRIVMSG message to send.
 * - client: Pointer to the client issuing the command.
 * - server: Pointer to the server instance managing channels.
 *
 * The function:
 * - Verifies the channel exists; sends ERR_NOSUCHCHANNEL (403) if not.
 * - Checks if the client is a member of the channel; sends ERR_CANNOTSENDTOCHAN (404) if not.
 * - Broadcasts the message to all channel members except the sender.
 */
static void processChannelMessage(const std::string& target, const std::string& fullMsg, Client* client, Server* server) {
    std::map<std::string, Channel*>& channels = server->getChannels();
    std::map<std::string, Channel*>::iterator chanIt = channels.find(target);
    if (chanIt == channels.end()) {
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_NOSUCHCHANNEL + " " +
                          client->getNickname() + " " + target + " :No such channel");
        return;
    }
    Channel* channel = chanIt->second;
    if (!channel->isMember(client)) {
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_CANNOTSENDTOCHAN + " " +
                          client->getNickname() + " " + target + " :Cannot send to channel");
        return;
    }
    channel->broadcast(fullMsg, client); // Exclude sender
}

/**
 * @brief Processes a PRIVMSG to a user as per RFC 2812 (Section 3.3.1).
 *
 * Parameters:
 * - target: The nickname of the recipient user.
 * - fullMsg: The complete PRIVMSG message to send.
 * - client: Pointer to the client issuing the command.
 * - server: Pointer to the server instance managing clients.
 *
 * The function:
 * - Searches for the target user among connected clients.
 * - Sends the message to the target if found; sends ERR_NOSUCHNICK (401) if not.
 */
static void processUserMessage(const std::string& target, const std::string& fullMsg, Client* client, Server* server) {
    std::map<int, Client*>& clients = server->getClients();
    bool found = false;
    for (std::map<int, Client*>::iterator clientIt = clients.begin(); clientIt != clients.end(); ++clientIt) {
        if (clientIt->second->getNickname() == target) {
            clientIt->second->sendReply(fullMsg);
            found = true;
            break;
        }
    }
    if (!found) {
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_NOSUCHNICK + " " +
                          client->getNickname() + " " + target + " :No such nick");
    }
}

/**
 * @brief Handles the PRIVMSG command as per RFC 2812 (Section 3.3.1).
 *
 * Parameters:
 * - cmdList: List of command arguments (target list and message).
 * - client: Pointer to the client issuing the command.
 * - server: Pointer to the server instance managing clients and channels.
 *
 * The function:
 * - Validates client registration and command parameters.
 * - Reconstructs the message from arguments, handling spaces.
 * - Splits target list by commas and processes each target (channel or user).
 * - Logs the PRIVMSG event.
 */
void handlePrivmsg(std::list<std::string> cmdList, Client* client, Server* server) {
    if (!CommandUtils::validateClientRegistration(client) || !validatePrivmsgParameters(cmdList, client)) {
        return;
    }

    std::list<std::string>::iterator it = cmdList.begin();
    ++it; // Skip "PRIVMSG"
    std::string targetsStr = *it;
    ++it;
    std::string message = *it;
    while (++it != cmdList.end()) {
        message += " " + *it; // Reconstruct message with spaces
    }

    if (!validateMessageLength(message, client)) {
        return;
    }

    std::list<std::string> targets = Utils::split(targetsStr, ',');
    std::string fullMsg = constructFullMessage(targetsStr, message, client);

    for (std::list<std::string>::iterator targetIt = targets.begin(); targetIt != targets.end(); ++targetIt) {
        std::string target = *targetIt;
        if (target[0] == '#') {
            processChannelMessage(target, fullMsg, client, server);
        } else {
            processUserMessage(target, fullMsg, client, server);
        }
    }

    Logger::info(client->getNickname() + " sent PRIVMSG to " + targetsStr + ": " + message);
}
