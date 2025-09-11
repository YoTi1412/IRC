#include "Command.hpp"
#include "Replies.hpp"

void Server::handlePass(std::list<std::string> cmdList, Client* client, Server* server) {
    if (cmdList.size() != 2) {
        client->sendReply(":ircserv " ERR_NEEDMOREPARAMS " * PASS :Not enough parameters\r\n");
        return;
    }
    if (client->isAuthenticated()) {
        client->sendReply(":ircserv " ERR_ALREADYREGISTRED " * :PASS already accepted, proceed with NICK and USER\r\n");
        return;
    }
    if (client->isNickSet() || client->isUserSet()) {
        client->sendReply(":ircserv " ERR_OUTOFORDER " * :PASS must be sent before NICK or USER\r\n");
        return;
    }

    std::list<std::string>::iterator it = cmdList.begin();
    ++it; // Skip "PASS"
    std::string pass = *it;

    if (pass != server->getPassword()) {
        client->sendReply(":ircserv " ERR_PASSWDMISMATCH " * :Password incorrect\r\n");
    } else {
        client->setAuthenticated();
        client->sendReply(":ircserv NOTICE AUTH :Password accepted\r\n");
    }
}
