#include "Command.hpp"
#include "Utils.hpp"
#include "Logger.hpp"
#include "Replies.hpp"

/**
 * @brief Validates the NICK command according to RFC 2812 (Section 3.1.2).
 *
 * Syntax (RFC 2812 §3.1.2):
 *   NICK <nickname>
 *
 * Behavior:
 * - Requires a nickname; else ERR_NONICKNAMEGIVEN (431).
 * - Nick changes allowed only if client is authenticated and not fully registered.
 * - PASS must precede NICK.
 * - NICK must be sent before USER for proper registration order.
 *
 * @param cmdList Parsed NICK command tokens.
 * @param client  The client issuing the command.
 * @return true if validation succeeds, false otherwise.
 */
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

/**
 * @brief Checks if a nickname conforms to allowed format per RFC 2812 (Section 3.1.2).
 *
 * Allowed characters: a-z, A-Z, 0-9, and []\\`_^{|}.
 *
 * @param nick Nickname string.
 * @return true if nickname format is valid, false otherwise.
 */
static bool isValidNickFormat(const std::string& nick) {
    static const std::string allowed = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789[]\\`_^{|}";
    return nick.find_first_not_of(allowed) == std::string::npos;
}

/**
 * @brief Checks if the nickname is already in use (RFC 2812 §3.1.2).
 *
 * Behavior:
 * - Case-insensitive comparison.
 * - Sends ERR_NICKNAMEINUSE (433) if nickname is taken.
 *
 * @param nick    Nickname to check.
 * @param client  Client attempting to use the nickname.
 * @param clients Map of all connected clients.
 * @return true if nickname is available, false otherwise.
 */
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

/**
 * @brief Updates client's nickname and sends appropriate notifications (RFC 2812 §3.1.2).
 *
 * - For nickname changes, sends ":oldNick!user@host NICK newNick".
 * - For initial nickname setting, sends a notice to the client.
 *
 * @param client Client whose nickname is being updated.
 * @param nick   New nickname.
 */
static void updateNickAndNotify(Client* client, const std::string& nick) {
    std::string oldNick = client->getNickname();
    client->setNickname(nick);
    client->setNickSet(true);

    if (!oldNick.empty() && client->isRegistered()) {
        client->sendReply(":" + oldNick + "!" + client->getUsername() + "@" + client->getHostname() + " NICK " + nick);
    } else {
        client->sendReply(IRC_SERVER " " NOTICE_JOIN " " + nick + " :Nickname set to " + nick);
    }
}

/**
 * @brief Completes client registration if all conditions are met (RFC 2812 §3.1.2).
 *
 * - Registration completes when PASS, NICK, and USER are set.
 * - Sends welcome messages: RPL_WELCOME (001), RPL_YOURHOST (002), RPL_CREATED (003).
 *
 * @param client Client to register.
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
 * @brief Handles the NICK command as specified in RFC 2812 (Section 3.1.2).
 *
 * Steps:
 * 1. Validate command parameters and client state.
 * 2. Check nickname format and availability.
 * 3. Update client's nickname and notify.
 * 4. Complete registration if all requirements are met.
 *
 * @param cmdList Parsed NICK command tokens.
 * @param client  The client issuing the command.
 * @param server  The IRC server instance.
 */
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
    maybeRegister(client, server);
}
