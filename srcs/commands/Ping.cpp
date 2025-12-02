#include "Includes.hpp"
#include <list>

void sendNeedMoreParamsError(Client* client, Server* server) {
    std::string nickname = CommandUtils::getNicknameOrDefault(client, "*");
    client->sendReply(":" + server->getName() + " " + ERR_NEEDMOREPARAMS + " " +
                      nickname + " PING :Not enough parameters");
}

std::string extractPingToken(std::list<std::string>& cmdList) {
    cmdList.pop_front();
    return cmdList.front();
}

void sendPongReply(Client* client, Server* server, const std::string& token) {
    client->sendReply(":" + server->getName() + " PONG " +
                      server->getName() + " :" + token);
}

void handlePing(std::list<std::string> cmdList, Client* client, Server* server) {
    if (cmdList.size() < 2) {
        sendNeedMoreParamsError(client, server);
        return;
    }

    std::string token = extractPingToken(cmdList);
    sendPongReply(client, server, token);
}
