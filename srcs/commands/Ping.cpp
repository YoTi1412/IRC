#include "Command.hpp"
#include "Replies.hpp"
#include <list>

/**
 * RFC 2812 §3.1 Registration / §3.7.2 PING
 * Sends ERR_NEEDMOREPARAMS when a client issues PING without parameters.
 */
void sendNeedMoreParamsError(Client* client, Server* server) {
    std::string nickname;
    if (client->getNickname().empty()) {
        nickname = "*";
    } else {
        nickname = client->getNickname();
    }

    client->sendReply(":" + server->getName() + " " + ERR_NEEDMOREPARAMS + " " +
                      nickname + " PING :Not enough parameters");
}

/**
 * RFC 2812 §3.7.2 PING
 * Extracts the PING token from the command list.
 */
std::string extractPingToken(std::list<std::string>& cmdList) {
    cmdList.pop_front(); // remove "PING"
    return cmdList.front();
}

/**
 * RFC 2812 §3.7.3 PONG
 * Sends a PONG reply back to the client with the given token.
 */
void sendPongReply(Client* client, Server* server, const std::string& token) {
    client->sendReply(":" + server->getName() + " PONG " +
                      server->getName() + " :" + token);
}

/**
 * RFC 2812 §3.7.2 PING
 * Handles the PING command from a client.
 * Used to test if a client connection is alive or to measure latency.
 */
void handlePing(std::list<std::string> cmdList, Client* client, Server* server) {
    if (cmdList.size() < 2) {
        sendNeedMoreParamsError(client, server);
        return;
    }

    std::string token = extractPingToken(cmdList);
    sendPongReply(client, server, token);
}
