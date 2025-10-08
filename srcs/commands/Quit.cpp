#include "Command.hpp"
#include "Replies.hpp"
#include "Logger.hpp"
#include <set>
#include <map>
#include <list>

/**
 * RFC 2812 §3.1 Registration
 * Error: Client must be registered before issuing QUIT.
 */
void sendNotRegisteredError(Client* client, Server* server) {
    client->sendReply(":" + server->getName() + " " + ERR_NOTREGISTERED + " * QUIT :You have not registered");
}

/**
 * RFC 2812 §3.1.7 QUIT
 * Extracts the quit message, defaulting to "Client Quit" if none is provided.
 */
std::string extractQuitMessage(std::list<std::string>& cmdList) {
    if (cmdList.size() > 1) {
        cmdList.pop_front();

        std::string message;
        for (std::list<std::string>::iterator it = cmdList.begin(); it != cmdList.end(); ++it) {
            if (!message.empty()) {
                message += " ";
            }
            message += *it;
        }
        return message;
    }
    return "Client Quit";
}

/**
 * RFC 2812 §3.1.7 QUIT
 * Builds the quit prefix to be broadcast to other clients.
 */
std::string buildQuitPrefix(Client* client, const std::string& message) {
    return ":" + client->getNickname() + "!" + client->getUsername() + "@" +
           client->getHostname() + " QUIT :" + message;
}

/**
 * RFC 2812 §3.2.1 Channel Membership
 * Finds all channels that the client is currently a member of.
 */
std::set<Channel*> findClientChannels(Client* client, Server* server) {
    std::set<Channel*> clientChannels;
    std::map<std::string, Channel*>& channels = server->getChannels();

    for (std::map<std::string, Channel*>::iterator it = channels.begin(); it != channels.end(); ++it) {
        if (it->second->isMember(client)) {
            clientChannels.insert(it->second);
        }
    }
    return clientChannels;
}

/**
 * RFC 2812 §3.1.7 QUIT
 * Broadcasts the QUIT message to all channels the client is in.
 */
void broadcastQuitToChannels(const std::set<Channel*>& channels, const std::string& prefix, Client* client) {
    for (std::set<Channel*>::iterator it = channels.begin(); it != channels.end(); ++it) {
        (*it)->broadcast(prefix, client);
    }
}

/**
 * RFC 2812 §3.1.7 QUIT
 * Sends ERROR message back to the quitting client before disconnecting.
 */
void sendErrorClosingLink(Client* client, const std::string& message) {
    client->sendReply("ERROR :Closing link: " + message);
}

/**
 * RFC 2812 §3.2.1 Channel Membership
 * Removes client from all channels and deletes empty channels from the server.
 */
void removeClientFromChannels(const std::set<Channel*>& channels, Client* client, Server* server) {
    for (std::set<Channel*>::iterator it = channels.begin(); it != channels.end(); ++it) {
        (*it)->removeMember(client);
        if ((*it)->getMemberCount() == 0) {
            server->removeChannel((*it)->getName());
        }
    }
}

/**
 * RFC 2812 §3.1.7 QUIT
 * Logs and disconnects the client from the server.
 */
void disconnectClient(Client* client, Server* server, const std::string& message) {
    int fd = client->getFd();
    Logger::info("Client " + client->getNickname() +
                 " (fd: " + Utils::intToString(fd) +
                 ") quit with message: " + message);
    server->handleClientDisconnect(fd);
}

/**
 * RFC 2812 §3.1.7 QUIT
 * Handles the QUIT command from a client.
 * A client session ends with a QUIT message.
 */
void handleQuit(std::list<std::string> cmdList, Client* client, Server* server) {
    if (!client->isRegistered()) {
        sendNotRegisteredError(client, server);
        return;
    }

    std::string message = extractQuitMessage(cmdList);
    std::string prefix = buildQuitPrefix(client, message);

    std::set<Channel*> clientChannels = findClientChannels(client, server);

    broadcastQuitToChannels(clientChannels, prefix, client);
    sendErrorClosingLink(client, message);
    removeClientFromChannels(clientChannels, client, server);
    disconnectClient(client, server, message);
}
