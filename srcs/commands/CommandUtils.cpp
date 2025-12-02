#include "Includes.hpp"
#include <sstream>

namespace CommandUtils {
    bool validateClientRegistration(Client* client) {
        if (!client->isRegistered()) {
            std::string nickname = getNicknameOrDefault(client, "*");
            client->sendReply(std::string(IRC_SERVER) + " " + ERR_NOTREGISTERED + " " +
                          nickname + " :You have not registered");
            return false;
        }
        return true;
    }

    bool validateParameters(std::list<std::string>& cmdList, Client* client, const std::string& command, size_t minParams) {
        if (cmdList.size() < minParams) {
            std::string nickname = getNicknameOrDefault(client, "*");
            client->sendReply(std::string(IRC_SERVER) + " " + ERR_NEEDMOREPARAMS + " " +
                          nickname + " " + command + " :Not enough parameters");
            return false;
        }
        return true;
    }

    Channel* getChannel(Server* server, const std::string& channelName, Client* client) {
        std::map<std::string, Channel*>::iterator it = server->getChannels().find(channelName);
        if (it == server->getChannels().end()) {
            std::ostringstream oss;
            oss << IRC_SERVER << " " << ERR_NOSUCHCHANNEL << " "
            << client->getNickname() << " " << channelName << " :No such channel";
            client->sendReply(oss.str());
            return NULL;
        }
        return it->second;
    }

    Client* getTargetClient(Server* server, Client* sender, const std::string& targetNick) {
        Client* target = server->getClientByNickname(targetNick);
        if (!target) {
            std::ostringstream oss;
            oss << IRC_SERVER << " " << ERR_NOSUCHNICK << " "
            << sender->getNickname() << " " << targetNick << " :No such nickname";
            sender->sendReply(oss.str());
            return NULL;
        }
        return target;
    }

    std::string getNicknameOrDefault(Client* client, const std::string& defaultNick) {
        if (client->getNickname().empty()) {
            return defaultNick;
        } else {
            return client->getNickname();
        }
    }
}
