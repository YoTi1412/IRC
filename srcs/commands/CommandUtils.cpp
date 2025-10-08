#include "Command.hpp"
#include "Utils.hpp"
#include "Logger.hpp"
#include "Replies.hpp"
#include "Channel.hpp"
#include <sstream>

namespace CommandUtils {

/**
 * @brief Validates if a client is registered.
 * 
 * This function checks if the client has completed the registration process.
 * If not registered, sends an appropriate error message.
 * 
 * @param client The client to validate
 * @return true if client is registered, false otherwise
 */
bool validateClientRegistration(Client* client) {
    if (!client->isRegistered()) {
        std::string nickname = getNicknameOrDefault(client, "*");
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_NOTREGISTERED + " " +
                          nickname + " :You have not registered");
        return false;
    }
    return true;
}

/**
 * @brief Validates command parameters.
 * 
 * Checks if the command has the minimum required number of parameters.
 * 
 * @param cmdList Command list containing parameters
 * @param client The client issuing the command
 * @param command The command name for error reporting
 * @param minParams Minimum number of parameters required
 * @return true if parameters are valid, false otherwise
 */
bool validateParameters(std::list<std::string>& cmdList, Client* client, const std::string& command, size_t minParams) {
    if (cmdList.size() < minParams) {
        std::string nickname = getNicknameOrDefault(client, "*");
        client->sendReply(std::string(IRC_SERVER) + " " + ERR_NEEDMOREPARAMS + " " +
                          nickname + " " + command + " :Not enough parameters");
        return false;
    }
    return true;
}

/**
 * @brief Gets a channel from the server by name.
 * 
 * Searches for a channel with the given name. If not found, sends an error message.
 * 
 * @param server The IRC server instance
 * @param channelName The name of the channel to find
 * @param client The client requesting the channel
 * @return Pointer to Channel if found, NULL otherwise
 */
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

/**
 * @brief Gets a target client by nickname.
 * 
 * Searches for a client with the given nickname. If not found, sends an error message.
 * 
 * @param server The IRC server instance
 * @param sender The client making the request
 * @param targetNick The nickname of the target client
 * @return Pointer to Client if found, NULL otherwise
 */
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

/**
 * @brief Gets client nickname or returns default value.
 * 
 * Helper function to safely get a client's nickname, returning a default
 * value if the nickname is empty.
 * 
 * @param client The client
 * @param defaultNick Default nickname to return if client's nickname is empty
 * @return Client's nickname or default value
 */
std::string getNicknameOrDefault(Client* client, const std::string& defaultNick) {
    return client->getNickname().empty() ? defaultNick : client->getNickname();
}

} // namespace CommandUtils