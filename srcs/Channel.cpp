#include "Includes.hpp"
#include "Channel.hpp"

Channel::Channel(const std::string& channelName, Client* creator)
    : name(channelName),
      topic(""),
      key(""),
      topicSetter(""),
      topicTime(0),
      inviteOnly(false),
      topicRestricted(false),
      limited(false),
      limit(0),
      secret(false)
{
    createdTime = Utils::formatTime(time(NULL));
    if (creator) {
        Logger::info("Channel " + name + " created by " + creator->getNickname());
    }
}

Channel::~Channel() {
    Logger::info("Channel " + name + " destroyed");
}

const std::string& Channel::getName() const { return name; }
const std::string& Channel::getTopic() const { return topic; }
const std::string& Channel::getKey() const { return key; }
bool Channel::getSecret() const { return secret; };
const std::string& Channel::getTopicSetter() const { return topicSetter; }
time_t Channel::getTopicTime() const { return topicTime; }
size_t Channel::getLimit() const { return limit; }
bool Channel::getInviteOnly() const { return inviteOnly; }
bool Channel::getTopicRestricted() const { return topicRestricted; }
bool Channel::getLimited() const { return limited; }
bool Channel::getKeyProtected() const { return !key.empty(); }
size_t Channel::getMemberCount() const { return members.size(); }

bool Channel::isMember(Client* client) const {
    if (!client) return false;
    return members.find(client->getFd()) != members.end();
}

bool Channel::isOperator(Client* client) const {
    if (!client) return false;
    return operators.find(client->getFd()) != operators.end();
}

bool Channel::isInvited(int fd) const {
    return std::find(inviteList.begin(), inviteList.end(), fd) != inviteList.end();
}

void Channel::setTopic(const std::string& newTopic, Client* setter) {
    if (setter && (!topicRestricted || isOperator(setter))) {
        for (std::string::const_iterator it = newTopic.begin(); it != newTopic.end(); ++it) {
            unsigned char c = static_cast<unsigned char>(*it);
            if (c < 32 || c > 126) {
                Logger::warning("Topic change failed for " + name + ": Invalid characters");
                return;
            }
        }
        topic = newTopic;
        topicSetter = setter->getNickname();
        topicTime = time(NULL);
        Logger::info("Topic set for " + name + " by " + topicSetter + ": " + newTopic);
    } else {
        Logger::warning("Topic change failed for " + name + ": Permission denied");
    }
}

void Channel::setKey(const std::string& newKey) {
    key = newKey;
    std::string action;
    if (key.empty()) {
        action = "removed from ";
    } else {
        action = "set for ";
    }
    Logger::info("Key " + action + name);
}

void Channel::setSecret(bool flag) {
    secret = flag;
    std::string status;
    if (flag) {
        status = "enabled";
    } else {
        status = "disabled";
    }
    Logger::info("Secret status " + status + " for " + name);
}

void Channel::setLimit(size_t newLimit) {
    limit = newLimit;
    limited = (newLimit > 0);
    std::string info;
    if (limited) {
        info = "set to " + Utils::intToString(newLimit);
    } else {
        info = "removed";
    }
    Logger::info("Limit " + info + " for " + name);
}

void Channel::setLimited(bool flag)
{
    limited = flag;
}

void Channel::setInviteOnly(bool flag) {
    inviteOnly = flag;
    std::string status;
    if (flag) {
        status = "enabled";
    } else {
        status = "disabled";
    }
    Logger::info("Invite-only " + status + " for " + name);
}

void Channel::setTopicRestricted(bool flag) {
    topicRestricted = flag;
    std::string status;
    if (flag) {
        status = "enabled";
    } else {
        status = "disabled";
    }
    Logger::info("Topic restriction " + status + " for " + name);
}

void Channel::addInvite(int fd) {
    if (!isInvited(fd)) {
        inviteList.push_back(fd);
        Logger::info("Client fd " + Utils::intToString(fd) + " invited to " + name);
    }
}

void Channel::removeInvite(int fd) {
    std::vector<int>::iterator it = std::find(inviteList.begin(), inviteList.end(), fd);
    if (it != inviteList.end()) {
        inviteList.erase(it);
        Logger::info("Invite removed for fd " + Utils::intToString(fd) + " from " + name);
    }
}

void Channel::addMember(Client* client) {
    if (client && !isMember(client)) {
        int fd = client->getFd();
        members[fd] = client;
        removeInvite(fd);
        if (members.size() == 1) {
            addOperator(fd);
        }
        Logger::info(client->getNickname() + " added to " + name);
    }
}

void Channel::removeMember(Client* client) {
    if (client && isMember(client)) {
        int fd = client->getFd();
        members.erase(fd);
        removeOperator(fd);
        removeInvite(fd);
        Logger::info(client->getNickname() + " removed from " + name);
    }
}

void Channel::addOperator(int fd) {
    if (members.find(fd) != members.end()) {
        operators.insert(fd);
        Logger::info("Client fd " + Utils::intToString(fd) + " promoted to operator in " + name);
    }
}

void Channel::removeOperator(int fd) {
    if (operators.find(fd) != operators.end()) {
        operators.erase(fd);
        Logger::info("Client fd " + Utils::intToString(fd) + " demoted from operator in " + name);
    }
}

void Channel::broadcast(const std::string& message, Client* sender) {
    for (std::map<int, Client*>::iterator it = members.begin(); it != members.end(); ++it) {
        if (it->second != sender) {
            it->second->sendReply(message);
        }
    }
    if (sender && isMember(sender)) {
        sender->sendReply(message);
    }
}

std::string Channel::getMemberList() const {
    std::string list;
    for (std::map<int, Client*>::const_iterator it = members.begin(); it != members.end(); ++it) {
        if (!list.empty()) list += " ";
        if (operators.find(it->first) != operators.end()) list += "@";
        list += it->second->getNickname();
    }
    return list;
}
