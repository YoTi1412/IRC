#include "Includes.hpp"


/**
 * @brief Basic INVITE command checks per RFC 2812 §3.2.7.
 *
 * Syntax (RFC 2812 §3.2.7):
 *   INVITE <nickname> <channel>
 *
 * Behaviour:
 * - Requires 2 parameters after the command.
 * - Client must be fully registered before issuing commands (RFC 2812 §3.1 Registration).
 *
 * Replies sent here:
 * - ERR_NEEDMOREPARAMS (461) when parameters are missing
 * - ERR_NOTREGISTERED (451) when the client is not registered
 */
static bool validateCommand(std::list<std::string>& cmdList, Client* client)
{
    if (!CommandUtils::validateParameters(cmdList, client, "INVITE", 3))
        return false;
    if (!CommandUtils::validateClientRegistration(client))
        return false;
    return true;
}

/**
 * @brief Resolve a channel name to a Channel object or reply with an error.
 *
 * RFC 2812 §3.2.7 requires the channel to exist; otherwise:
 * - ERR_NOSUCHCHANNEL (403)
 */


/**
 * @brief Wrapper for INVITE command validation.
 *
 * Currently defers to validateCommand(); kept for symmetry with other commands
 * and to centralize INVITE-specific checks if extended later (RFC 2812 §3.2.7).
 */
static bool validateInviteCommand(std::list<std::string>& cmdList, Client* client)
{
    if (!validateCommand(cmdList, client))
        return false;
    return true;
}

/**
 * @brief Ensure inviter is on the channel and has privileges where required.
 *
 * RFC 2812 §3.2.7:
 * - The inviter MUST be on the channel, else ERR_NOTONCHANNEL (442).
 * - For invite-only (+i) channels, only channel operators may INVITE,
 *   else ERR_CHANOPRIVSNEEDED (482).
 */
static bool validateChannelAndPermissions(Channel* channel, Client* client)
{
    if (!channel->isMember(client))
    {
        std::ostringstream oss;
        oss << IRC_SERVER << " " << ERR_NOTONCHANNEL << " "
            << client->getNickname() << " " << channel->getName() << " :You're not on that channel";
        client->sendReply(oss.str());
        return false;
    }

    if (channel->getInviteOnly() && !channel->isOperator(client))
    {
        std::ostringstream oss;
        oss << IRC_SERVER << " " << ERR_CHANOPRIVSNEEDED << " "
            << client->getNickname() << " " << channel->getName() << " :You're not channel operator";
        client->sendReply(oss.str());
        return false;
    }
    return true;
}

/**
 * @brief Resolve target nickname to a connected client.
 *
 * RFC 2812 §3.2.7:
 * - ERR_NOSUCHNICK (401) if the target nickname does not exist.
 */


/**
 * @brief Ensure target is not already a member of the channel.
 *
 * RFC 2812 §3.2.7:
 * - ERR_USERONCHANNEL (443) if the target is already on the channel.
 */
static bool checkTargetNotInChannel(Channel* channel, Client* sender, Client* target)
{
    if (channel->isMember(target))
    {
        std::ostringstream oss;
        oss << IRC_SERVER << " " << ERR_USERONCHANNEL << " "
            << sender->getNickname() << " " << target->getNickname() << " " 
            << channel->getName() << " :is already on channel";
        sender->sendReply(oss.str());
        return false;
    }
    return true;
}

/**
 * @brief Send the INVITE message to the target (and echo to inviter).
 *
 * RFC 2812 §3.2.7:
 * - The server SHOULD notify the target with an INVITE.
 * - Many servers also send RPL_INVITING (341) to the inviter; this
 *   implementation currently echoes the INVITE message to the inviter instead.
 */
static void sendInvite(Channel* channel, Client* sender, Client* target)
{
    channel->addInvite(target->getFd());
    std::ostringstream oss;
    oss << ":" << sender->getNickname() << "!" << sender->getUsername()
        << "@" << sender->getHostname()
        << " INVITE " << target->getNickname() << " " << channel->getName();
    target->sendReply(oss.str());
    sender->sendReply(oss.str());
    Logger::info(sender->getNickname() + " invited " + target->getNickname() + " to " + channel->getName());
}

/**
 * @brief Ensure the client is registered before processing INVITE.
 *
 * Registration requirement is aligned with RFC 2812 §3.1 Registration.
 * Replies:
 * - ERR_NOTREGISTERED (451) if not registered.
 */


/**
 * @brief Handle INVITE as specified in RFC 2812 §3.2.7.
 *
 * Flow:
 * - Validate syntax and registration
 * - Check channel existence and inviter privileges
 * - Resolve target nickname
 * - Ensure target not already on channel
 * - Send INVITE to target and echo to inviter
 *
 * Possible replies involved:
 * - ERR_NEEDMOREPARAMS (461), ERR_NOTREGISTERED (451)
 * - ERR_NOSUCHCHANNEL (403), ERR_NOTONCHANNEL (442)
 * - ERR_CHANOPRIVSNEEDED (482), ERR_NOSUCHNICK (401), ERR_USERONCHANNEL (443)
 * - Optionally RPL_INVITING (341) to acknowledge invitation (not currently sent)
 */
void handleInvite(std::list<std::string> cmdList, Client* client, Server* server)
{
    if (!validateInviteCommand(cmdList, client))
        return;

    std::list<std::string>::iterator it = cmdList.begin();
    ++it;
    std::string channelName = *it;
    Channel* channel = CommandUtils::getChannel(server, channelName, client);
    if (!channel || !validateChannelAndPermissions(channel, client))
        return;

    ++it;
    std::string targetNick = *it;
    Client* target = CommandUtils::getTargetClient(server, client, targetNick);
    if (!target || !checkTargetNotInChannel(channel, client, target))
        return;

    sendInvite(channel, client, target);
}
