#include "Command.hpp"
#include "Utils.hpp"
#include "Logger.hpp"
#include "Replies.hpp"
#include <sstream>

// RFC 2812: Section 3.3.1 - Validates client registration for PRIVMSG command
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

// RFC 2812: Section 3.3.1 - Checks if PRIVMSG command has sufficient parameters
static bool validatePrivmsgParameters(std::list<std::string>& cmdList, Client* client) {
    if (cmdList.size() < 2) {
        std::string nickname;
        if (client->getNickname().empty()) {
            nickname = "*";
        } else {
            nickname = client->getNickname();
        }
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_NORECIPIENT + " " +
                          nickname + " :No recipient given (PRIVMSG)");
        return false;
    }
    if (cmdList.size() < 3) {
        std::string nickname;
        if (client->getNickname().empty()) {
            nickname = "*";
        } else {
            nickname = client->getNickname();
        }
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_NOTEXTTOSEND + " " +
                          nickname + " :No text to send");
        return false;
    }
    return true;
}

// RFC 2812: Section 3.3.1 - Validates message length including CRLF
static bool validateMessageLength(const std::string& message, Client* client) {
    std::string fullMsgWithCRLF = message + "\r\n";
    if (fullMsgWithCRLF.length() > MAX_MESSAGE_LENGTH) {
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_TOOMANYCHANNELS + " " +
                          client->getNickname() + " :Message too long");
        return false;
    }
    return true;
}

// RFC 2812: Section 3.3.1 - Constructs the full PRIVMSG message with user identifier
static std::string constructFullMessage(const std::string& targetsStr, const std::string& message, Client* client) {
    std::string userIdent = client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname();
    return ":" + userIdent + " PRIVMSG " + targetsStr + " :" + message;
}

// RFC 2812: Section 3.3.1 - Processes message to a channel
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

// RFC 2812: Section 3.3.1 - Processes message to a user
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

// RFC 2812: Section 3.3.1 - Handles PRIVMSG command by processing targets and sending messages
void handlePrivmsg(std::list<std::string> cmdList, Client* client, Server* server) {
    if (!validateClientRegistration(client) || !validatePrivmsgParameters(cmdList, client)) {
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
