#pragma once

#include "Includes.hpp"
#include "Client.hpp"
#include "Utils.hpp"
#include "Logger.hpp"

#define MAX_MESSAGE_LENGTH 512

class Client;

class Channel {
private:
    Channel();
    std::string name;
    std::string topic;
    std::string key;
    std::string topicSetter;
    time_t topicTime;

    std::map<int, Client*> members; // fd -> Client*
    std::set<int> operators;        // subset of fds
    std::vector<int> inviteList;    // invited fds

    bool inviteOnly;
    bool topicRestricted;
    bool limited;
    size_t limit;

    std::string createdTime;

public:
    Channel(const std::string& channelName, Client* creator);
    ~Channel();

    // getters
    const std::string& getName() const;
    const std::string& getTopic() const;
    const std::string& getKey() const;
    const std::string& getTopicSetter() const;
    time_t getTopicTime() const;
    size_t getLimit() const;
    bool getInviteOnly() const;
    bool getTopicRestricted() const;
    bool getLimited() const;
    bool getKeyProtected() const;
    size_t getMemberCount() const;

    // membership
    bool isMember(Client* client) const;
    bool isOperator(Client* client) const;
    bool isInvited(int fd) const;

    // setters
    void setTopic(const std::string& newTopic, Client* setter);
    void setKey(const std::string& newKey);
    void setLimit(size_t newLimit);
    void setInviteOnly(bool flag);
    void setTopicRestricted(bool flag);
    void setLimited(bool flag);

    // invites
    void addInvite(int fd);
    void removeInvite(int fd);

    // members
    void addMember(Client* client);
    void removeMember(Client* client);
    void addOperator(int fd);
    void removeOperator(int fd);

    // broadcast
    void broadcast(const std::string& message, Client* sender);

    // utils
    std::string getMemberList() const;
};
