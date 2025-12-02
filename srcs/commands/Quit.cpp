#include "Includes.hpp"

void sendNotRegisteredError(Client* client, Server* server) {
    client->sendReply(":" + server->getName() + " " + ERR_NOTREGISTERED + " * QUIT :You have not registered");
}

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

std::string buildQuitPrefix(Client* client, const std::string& message) {
    return ":" + client->getNickname() + "!" + client->getUsername() + "@" +
           client->getHostname() + " QUIT :" + message;
}

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

void broadcastQuitToChannels(const std::set<Channel*>& channels, const std::string& prefix, Client* client) {
    for (std::set<Channel*>::iterator it = channels.begin(); it != channels.end(); ++it) {
        (*it)->broadcast(prefix, client);
    }
}

void sendErrorClosingLink(Client* client, const std::string& message) {
    client->sendReply("ERROR :Closing link: " + message);
}

void removeClientFromChannels(const std::set<Channel*>& channels, Client* client, Server* server) {
    for (std::set<Channel*>::iterator it = channels.begin(); it != channels.end(); ++it) {
        (*it)->removeMember(client);
        if ((*it)->getMemberCount() == 0) {
            server->removeChannel((*it)->getName());
        }
    }
}

void disconnectClient(Client* client, Server* server, const std::string& message) {
    int fd = client->getFd();
    Logger::info("Client " + client->getNickname() +
                 " (fd: " + Utils::intToString(fd) +
                 ") quit with message: " + message);
    server->handleClientDisconnect(fd);
}

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
