#include "Command.hpp"
#include "Utils.hpp"
#include "Logger.hpp"
#include "Replies.hpp"
#include <sstream>

static bool validatePrivmsg(std::list<std::string>& cmdList, Client* client) {
    if (!client->isRegistered()) {
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_NOTREGISTERED + " " +
                          (client->getNickname().empty() ? "*" : client->getNickname()) +
                          " :You have not registered");
        return false;
    }
    if (cmdList.size() < 3) { // At least command, target, and message
        if (cmdList.size() < 2) {
            client->sendReply(std::string(IRC_SERVER) + " " + ERR_NORECIPIENT + " " +
                              (client->getNickname().empty() ? "*" : client->getNickname()) +
                              " :No recipient given (PRIVMSG)");
        } else {
            client->sendReply(std::string(IRC_SERVER) + " " + ERR_NOTEXTTOSEND + " " +
                              (client->getNickname().empty() ? "*" : client->getNickname()) +
                              " :No text to send");
        }
        return false;
    }
    return true;
}

void handlePrivmsg(std::list<std::string> cmdList, Client* client, Server* server) {
    if (!validatePrivmsg(cmdList, client)) {
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

    // Check message length including CRLF (approximated here)
    std::string fullMsgWithCRLF = message + "\r\n";
    if (fullMsgWithCRLF.length() > MAX_MESSAGE_LENGTH) {
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_TOOMANYCHANNELS + " " + // Reuse 405 for length, adjust if needed
                          client->getNickname() + " :Message too long");
        return;
    }

    std::list<std::string> targets = Utils::split(targetsStr, ',');
    std::string userIdent = client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname();
    std::string fullMsg = ":" + userIdent + " PRIVMSG " + targetsStr + " :" + message;

    for (std::list<std::string>::iterator targetIt = targets.begin(); targetIt != targets.end(); ++targetIt) {
        std::string target = *targetIt;

        if (target[0] == '#') { // Channel message
            std::map<std::string, Channel*>& channels = server->getChannels();
            std::map<std::string, Channel*>::iterator chanIt = channels.find(target);
            if (chanIt == channels.end()) {
                client->sendReply(std::string(IRC_SERVER) + " " + ERR_NOSUCHCHANNEL + " " +
                                  client->getNickname() + " " + target + " :No such channel");
                continue;
            }
            Channel* channel = chanIt->second;
            if (!channel->isMember(client)) {
                client->sendReply(std::string(IRC_SERVER) + " " + ERR_CANNOTSENDTOCHAN + " " +
                                  client->getNickname() + " " + target + " :Cannot send to channel");
                continue;
            }
            channel->broadcast(fullMsg, client); // Exclude sender
        } else { // User message
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
    }

    Logger::info(client->getNickname() + " sent PRIVMSG to " + targetsStr + ": " + message);
}
