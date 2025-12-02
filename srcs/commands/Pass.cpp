#include "Includes.hpp"

static bool validatePass(std::list<std::string>& cmdList, Client* client) {
    if (cmdList.size() != 2) {
        client->sendReply(IRC_SERVER " " ERR_NEEDMOREPARAMS " * PASS :Not enough parameters");
        return false;
    }
    if (client->isAuthenticated()) {
        client->sendReply(IRC_SERVER " " ERR_ALREADYREGISTRED " * :PASS already accepted, proceed with NICK and USER");
        return false;
    }
    if (client->isNickSet() || client->isUserSet()) {
        client->sendReply(IRC_SERVER " " ERR_OUTOFORDER " * :PASS must be sent before NICK or USER");
        return false;
    }
    return true;
}

static void processPass(const std::string& pass, Client* client, Server* server) {
    if (pass != server->getPassword()) {
        client->sendReply(IRC_SERVER " " ERR_PASSWDMISMATCH " * :Password incorrect");
    } else {
        client->setAuthenticated(true);
        client->sendReply(IRC_SERVER " " NOTICE_AUTH " :Password accepted");
    }
}

void handlePass(std::list<std::string> cmdList, Client* client, Server* server) {
    if (!validatePass(cmdList, client))
        return;

    std::list<std::string>::iterator it = cmdList.begin();
    ++it;
    std::string pass = *it;

    processPass(pass, client, server);
}
