#include "Command.hpp"
#include "Utils.hpp"
#include "Logger.hpp"
#include "Replies.hpp"

static bool validateNickCommand(std::list<std::string>& cmdList, Client* client) {
    if (cmdList.size() < 2) {
        client->sendReply(IRC_SERVER " " ERR_NONICKNAMEGIVEN " * :No nickname given");
        return false;
    }
    if (!client->isAuthenticated() && client->isRegistered()) {
        client->sendReply(IRC_SERVER " " ERR_ALREADYREGISTRED " " + client->getNickname() + " :Unauthorized command (already registered)");
        return false;
    }
    if (!client->isAuthenticated()) {
        client->sendReply(IRC_SERVER " " ERR_ALREADYREGISTRED " * :You must send PASS before NICK");
        return false;
    }
    if (client->isUserSet() && !client->isRegistered()) {
        client->sendReply(IRC_SERVER " " ERR_OUTOFORDER " * :NICK must be sent before USER");
        return false;
    }
    return true;
}

static bool isValidNickFormat(const std::string& nick) {
    // allowed characters per your previous code
    static const std::string allowed = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789[]\\`_^{|}";
    return nick.find_first_not_of(allowed) == std::string::npos;
}

static bool isNickAvailable(const std::string& nick, Client* client, std::map<int, Client*>& clients) {
    for (std::map<int, Client*>::iterator it = clients.begin(); it != clients.end(); ++it) {
        Client* other = it->second;
        if (other == client) continue;
        if (Utils::toLower(other->getNickname()) == Utils::toLower(nick)) {
            client->sendReply(IRC_SERVER " " ERR_NICKNAMEINUSE " * " + nick + " :Nickname is already in use");
            return false;
        }
    }
    return true;
}

static void updateNickAndNotify(Client* client, const std::string& nick) {
    std::string oldNick = client->getNickname();
    client->setNickname(nick);
    client->setNickSet(true);

    if (!oldNick.empty() && client->isRegistered()) {
        // announce nick change to the client
        client->sendReply(":" + oldNick + "!" + client->getUsername() + "@" + client->getHostname() + " NICK " + nick);
    } else {
        client->sendReply(IRC_SERVER " " NOTICE_JOIN " " + nick + " :Nickname set to " + nick);
    }
}

static void maybeRegister(Client* client) {
    if (client->isAuthenticated() && client->isNickSet() && client->isUserSet() && !client->isRegistered()) {
        client->setRegistered(true);
        std::string nick = client->getNickname();
        client->sendReply(IRC_SERVER " " RPL_WELCOME " " + nick + " :Welcome to the IRC server!");
        client->sendReply(IRC_SERVER " " RPL_YOURHOST " " + nick + " :Your host is ircserv, running version 1.0");
        client->sendReply(IRC_SERVER " " RPL_CREATED " " + nick + " :This server was created on " + Utils::getFormattedTime());
    }
}

void handleNick(std::list<std::string> cmdList, Client* client, Server* server) {
    if (!validateNickCommand(cmdList, client)) return;

    std::list<std::string>::iterator it = cmdList.begin();
    ++it; // skip "NICK"
    std::string nick = *it;

    if (!isValidNickFormat(nick)) {
        client->sendReply(IRC_SERVER " " ERR_ERRONEUSNICKNAME " * " + nick + " :Erroneous nickname");
        return;
    }

    std::map<int, Client*>& clients = server->getClients();
    if (!isNickAvailable(nick, client, clients)) return;

    updateNickAndNotify(client, nick);
    maybeRegister(client);
}
