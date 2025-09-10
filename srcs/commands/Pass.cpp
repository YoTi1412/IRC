#include "Command.hpp"

void handlePass(std::list<std::string> cmdList, Client* client, Server* server) {
    if (cmdList.size() != 2) {
        client->sendReply(":ircserv 461 * PASS :Not enough parameters\r\n"); // RFC 2812: ERR_NEEDMOREPARAMS
        return;
    }

    std::list<std::string>::iterator it = cmdList.begin();
    ++it; // Skip "PASS"
    std::string pass = *it;

    if (pass != server->getPassword()) {  // Check against Server's password
        client->sendReply(":ircserv 464 * :Password incorrect\r\n"); // RFC 2812: ERR_PASSWDMISMATCH
    } else {
        client->setAuthenticated(); // Changed to setAuthenticated
        client->sendReply(":ircserv NOTICE AUTH :Password accepted\r\n");
    }
}
