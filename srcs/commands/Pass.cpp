#include "Includes.hpp"

/**
 * Validates the PASS command as specified in RFC 2812 (Section 3.1.1).
 *
 * Syntax:
 *   PASS <password>
 *
 * Behavior:
 * - Requires exactly one parameter.
 *   • If missing → ERR_NEEDMOREPARAMS (461).
 * - If the client is already authenticated:
 *   • ERR_ALREADYREGISTRED (462) is returned.
 * - PASS must precede NICK and USER.
 *   • If NICK or USER already set → ERR_OUTOFORDER.
 *
 * @param cmdList Parsed PASS command tokens.
 * @param client  The client issuing the command.
 * @return true if validation succeeds, false otherwise.
 */
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

/**
 * Processes the PASS command
 *
 * Behavior:
 * - Compares supplied password with the server’s configured password.
 *   • If mismatch → ERR_PASSWDMISMATCH (464).
 *   • If match → set authenticated flag and acknowledge success.
 *
 * @param pass   Password supplied by the client.
 * @param client The client issuing the command.
 * @param server The IRC server instance.
 */
static void processPass(const std::string& pass, Client* client, Server* server) {
    if (pass != server->getPassword()) {
        client->sendReply(IRC_SERVER " " ERR_PASSWDMISMATCH " * :Password incorrect");
    } else {
        client->setAuthenticated(true);
        client->sendReply(IRC_SERVER " " NOTICE_AUTH " :Password accepted");
    }
}

/**
 * Handles the PASS command
 *
 * Steps:
 * 1. Validate command syntax and client state (see validatePass()).
 * 2. Process the supplied password (see processPass()).
 *
 * Reference: RFC 2812 Section 3.1.1
 *
 * @param cmdList Parsed PASS command tokens.
 * @param client  The client issuing the command.
 * @param server  The IRC server instance.
 */
void handlePass(std::list<std::string> cmdList, Client* client, Server* server) {
    if (!validatePass(cmdList, client))
        return;

    std::list<std::string>::iterator it = cmdList.begin();
    ++it; // move to the password
    std::string pass = *it;

    processPass(pass, client, server);
}
