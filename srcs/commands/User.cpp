#include "Command.hpp"
#include "Utils.hpp"
#include "Logger.hpp"
#include "Replies.hpp"

static bool validateUserCommand(std::list<std::string>& cmdList, Client* client) {
    if (cmdList.size() != 5) {
        client->sendReply(IRC_SERVER " " ERR_NEEDMOREPARAMS " * USER :Not enough parameters");
        return false;
    }
    if (client->isRegistered()) {
        client->sendReply(IRC_SERVER " " ERR_ALREADYREGISTRED " " + client->getNickname() + " :Unauthorized command (already registered)");
        return false;
    }
    if (!client->isAuthenticated()) {
        client->sendReply(IRC_SERVER " " ERR_ALREADYREGISTRED " * :You must send PASS before USER");
        return false;
    }
    if (!client->isNickSet()) {
        client->sendReply(IRC_SERVER " " ERR_ALREADYREGISTRED " * :You must send NICK before USER");
        return false;
    }
    return true;
}

static void parseUserCommand(std::list<std::string>& cmdList, std::string& username, std::string& realname) {
    std::list<std::string>::iterator it = cmdList.begin();
    ++it; // skip "USER"
    username = *it;
    ++it; // mode
    ++it; // unused
    ++it; // realname (note: realname may include leading ':' in original raw message)
    realname = *it;
}

static bool isUsernameAvailable(const std::string& username, Client* client, std::map<int, Client*>& clients) {
    for (std::map<int, Client*>::iterator it = clients.begin(); it != clients.end(); ++it) {
        Client* other = it->second;
        if (other == client) continue;
        if (Utils::toLower(other->getUsername()) == Utils::toLower(username)) {
            client->sendReply(IRC_SERVER " " ERR_ALREADYREGISTRED " * " + username + " :Username is already in use");
            return false;
        }
    }
    return true;
}

static void setUserAndMaybeRegister(Client* client, const std::string& username, const std::string& realname, Server* server) {
    (void)realname; // currently unused beyond storing username/realname if you choose to store it
    client->setUsername(username);
    client->setHostname(client->getIPAddress());
    client->setUserSet(true);

    if (client->isAuthenticated() && client->isNickSet() && client->isUserSet()) {
        client->setRegistered(true);
        std::string nick = client->getNickname();
        client->sendReply(IRC_SERVER " " RPL_WELCOME " " + nick + " :Welcome to the IRC server!");
        client->sendReply(IRC_SERVER " " RPL_YOURHOST " " + nick + " :Your host is ircserv, running version 1.0");
        client->sendReply(IRC_SERVER " " RPL_CREATED " " + nick + " :This server was created on " + server->getCreatedTime());
    }
}

/* Public handler signature:
   void handleUser(std::list<std::string> cmdList, Client* client, Server* server);
*/
void handleUser(std::list<std::string> cmdList, Client* client, Server* server) {
    if (!validateUserCommand(cmdList, client)) return;

    std::string username;
    std::string realname;
    parseUserCommand(cmdList, username, realname);

    // assume Server has a method getClients() that returns std::map<int, Client*>&
    std::map<int, Client*>& clients = server->getClients();
    if (!isUsernameAvailable(username, client, clients)) return;

    setUserAndMaybeRegister(client, username, realname, server);
}
