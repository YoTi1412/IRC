#include "Command.hpp"
#include "Utils.hpp"
#include "Logger.hpp"
#include "Replies.hpp"
#include "Channel.hpp"
#include <sstream>  // For std::ostringstream

static bool validateJoin(std::list<std::string>& cmdList, Client* client) {
    if (cmdList.size() < 2) {
        std::ostringstream oss;
        oss << IRC_SERVER << " " << ERR_NEEDMOREPARAMS << " " << client->getNickname() << " JOIN :Not enough parameters";
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

static void processJoin(const std::string& channelName, Client* client, Server* server) {
    if (channelName.empty() || channelName[0] != '#') {
        std::ostringstream oss;
        oss << IRC_SERVER << " " << ERR_BADCHANMASK << " " << client->getNickname() << " " << channelName << " :Bad Channel Mask";
        client->sendReply(oss.str());
        return;
    }

    std::map<std::string, Channel*>& channels = server->getChannels();
    Channel* channel = NULL;
    std::map<std::string, Channel*>::iterator it = channels.find(channelName);
    if (it == channels.end()) {
        channel = new Channel(channelName, client);
        channels[channelName] = channel;
    } else {
        channel = it->second;
        if (channel->isMember(client)) {
            return; // Already in channel
        }
        if (channel->getInviteOnly() && !channel->isInvited(client->getFd())) {
            std::ostringstream oss;
            oss << IRC_SERVER << " " << ERR_INVITEONLYCHAN << " " << client->getNickname() << " " << channelName << " :Cannot join channel (+i)";
            client->sendReply(oss.str());
            return;
        }
        // TODO: Add key check (ERR_BADCHANNELKEY if key mismatch)
        if (channel->getLimited() && channel->getLimit() <= channel->getMemberCount()) {
            std::ostringstream oss;
            oss << IRC_SERVER << " " << ERR_CHANNELISFULL << " " << client->getNickname() << " " << channelName << " :Cannot join channel (+l)";
            client->sendReply(oss.str());
            return;
        }
        channel->addMember(client);
    }

    // Build user ident
    std::string userIdent = client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname();

    // Broadcast JOIN
    std::string joinMsg = ":" + userIdent + " JOIN " + channelName;
    channel->broadcast(joinMsg, client);

    // Send topic if set
    if (!channel->getTopic().empty()) {
        std::ostringstream oss;
        oss << IRC_SERVER << " " << RPL_TOPIC << " " << client->getNickname() << " " << channelName << " :" << channel->getTopic();
        client->sendReply(oss.str());
    }

    // Send names list
    {
        std::ostringstream oss;
        oss << IRC_SERVER << " " << RPL_NAMREPLY << " " << client->getNickname() << " = " << channelName << " :" << channel->getMemberList();
        client->sendReply(oss.str());
    }
    {
        std::ostringstream oss;
        oss << IRC_SERVER << " " << RPL_ENDOFNAMES << " " << client->getNickname() << " " << channelName << " :End of NAMES list";
        client->sendReply(oss.str());
    }

    Logger::info(client->getNickname() + " joined " + channelName);
}

void handleJoin(std::list<std::string> cmdList, Client* client, Server* server) {
    if (!validateJoin(cmdList, client)) return;

    std::list<std::string>::iterator it = cmdList.begin();
    ++it; // Skip "JOIN"
    std::string channelName = *it; // Handle single channel for now

    processJoin(channelName, client, server);
}
