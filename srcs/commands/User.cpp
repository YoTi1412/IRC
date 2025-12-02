#include "Includes.hpp"

static bool validateUserCommand(std::list<std::string>& cmdList, Client* client) {
    if (cmdList.size() < 5) {
        client->sendReply(IRC_SERVER " " ERR_NEEDMOREPARAMS " * USER :Not enough parameters");
        return false;
    }
    if (!client->isAuthenticated()) {
        client->sendReply(IRC_SERVER " " ERR_OUTOFORDER " * :You must send PASS before USER");
        return false;
    }
    if (client->isRegistered()) {
        client->sendReply(IRC_SERVER " " ERR_ALREADYREGISTRED " " + client->getNickname() + " :Unauthorized command (already registered)");
        return false;
    }
    if (client->isUserSet()) {
        client->sendReply(IRC_SERVER " " ERR_ALREADYREGISTRED " * :USER already set");
        return false;
    }
    if (!client->isNickSet()) {
        client->sendReply(IRC_SERVER " " ERR_OUTOFORDER " * :NICK must be sent before USER");
        return false;
    }
    return true;
}

static void maybeRegister(Client* client, Server* server) {
    if (client->isAuthenticated() && client->isNickSet() && client->isUserSet() && !client->isRegistered()) {
        client->setRegistered(true);
        std::string nick = client->getNickname();
        std::string userIdent = nick + "!" + client->getUsername() + "@" + client->getHostname();
        client->sendReply(IRC_SERVER " " RPL_WELCOME " " + nick + " :Welcome to the Internet Relay Network " + userIdent);
        client->sendReply(IRC_SERVER " " RPL_YOURHOST " " + nick + " :Your host is ircserv, running version 1.0");
        client->sendReply(IRC_SERVER " " RPL_CREATED " " + nick + " :This server was created " + server->getCreatedTime());
        client->sendReply(IRC_SERVER " " RPL_MYINFO " " + nick + " ircserv 1.0 " "" " itkol");
    }
}

void handleUser(std::list<std::string> cmdList, Client* client, Server* server) {
    if (!validateUserCommand(cmdList, client)) return;

    std::list<std::string>::iterator it = cmdList.begin();
    ++it;
    std::string username = *it;

    ++it;
    std::string modeStr = *it;
    if (modeStr != "0") {
        client->sendReply(IRC_SERVER " " ERR_NEEDMOREPARAMS " * USER :Mode must be 0");
        return;
    }

    ++it;
    ++it;

    std::string realname = *it;
    if (realname[0] == ':') {
        realname = realname.substr(1);
    } else {

        std::list<std::string>::iterator next = it;
        ++next;
        if (next != cmdList.end()) {
            client->sendReply(IRC_SERVER " " ERR_NEEDMOREPARAMS " * USER :Use : for multi-word realnames");
            return;
        }
    }

    client->setUsername(username);
    client->setHostname(client->getIPAddress());
    client->setRealname(realname);
    client->setUserSet(true);
    maybeRegister(client, server);
}
