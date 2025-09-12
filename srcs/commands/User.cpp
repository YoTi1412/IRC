#include "Command.hpp"
#include "Utils.hpp"
#include "Logger.hpp"
#include "Replies.hpp"

/**
 * @brief Validates the USER command according to RFC 2812 (Section 3.1.3).
 *
 * Syntax (RFC 2812 §3.1.3):
 *   USER <username> <mode> <unused> <realname>
 *
 * Behaviour:
 * - Requires exactly 4 parameters (total 5 tokens incl. "USER").
 * - Client must be authenticated (PASS already sent).
 * - NICK must already have been sent.
 * - USER may be sent only once and only before registration is complete.
 *
 * Replies:
 * - ERR_NEEDMOREPARAMS (461)  if any parameter is missing
 * - ERR_OUTOFORDER            custom code for order violations
 * - ERR_ALREADYREGISTRED (462) if registration has already finished
 */
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

/**
 * @brief Completes client registration if all conditions are met.
 *
 * Registration finishes when:
 * - PASS accepted (authenticated)
 * - NICK successfully set
 * - USER successfully set
 *
 * Upon success the server sends:
 * - RPL_WELCOME  (001)
 * - RPL_YOURHOST (002)
 * - RPL_CREATED  (003)
 * as specified in RFC 2812 §5.1.
 */
static void maybeRegister(Client* client, Server* server) {
    if (client->isAuthenticated() && client->isNickSet() && client->isUserSet() && !client->isRegistered()) {
        client->setRegistered(true);
        std::string nick = client->getNickname();
        std::string userIdent = nick + "!" + client->getUsername() + "@" + client->getHostname();
        client->sendReply(IRC_SERVER " " RPL_WELCOME " " + nick + " :Welcome to the Internet Relay Network " + userIdent);
        client->sendReply(IRC_SERVER " " RPL_YOURHOST " " + nick + " :Your host is ircserv, running version 1.0");
        client->sendReply(IRC_SERVER " " RPL_CREATED " " + nick + " :This server was created " + server->getCreatedTime());
    }
}

/**
 * @brief Handles the USER command as specified in RFC 2812 (Section 3.1.3).
 *
 * Parameters:
 * 1. <username> - login name (stored)
 * 2. <mode>     - bitmask for initial user modes (ignored except "0" enforced here)
 * 3. <unused>   - historical field, totally ignored
 * 4. <realname> - human-readable name (stored)
 *
 * The server:
 * - Validates parameter count and command order
 * - Enforces <mode> == "0" (project decision, not mandated by RFC)
 * - Strips leading ':' from <realname> if present
 * - Completes registration when NICK+USER+PASS are all satisfied
 */
void handleUser(std::list<std::string> cmdList, Client* client, Server* server) {
    if (!validateUserCommand(cmdList, client)) return;

    std::list<std::string>::iterator it = cmdList.begin();
    ++it; // Skip "USER"
    std::string username = *it;

    ++it; // mode
    std::string modeStr = *it;
    if (modeStr != "0") {
        client->sendReply(IRC_SERVER " " ERR_NEEDMOREPARAMS " * USER :Mode must be 0");
        return;
    }

    ++it; // Skip unused
    ++it; // Move to realname token

    std::string realname = *it;
    if (realname[0] == ':') {
        realname = realname.substr(1); // Strip ':' and use rest of line
    } else {
        // RFC 2812 allows a single-token realname; if more tokens exist
        // the client should have used ':' prefix.
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
