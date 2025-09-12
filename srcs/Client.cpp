#include "Client.hpp"
#include "Utils.hpp"
#include "Logger.hpp"

// Constructor
Client::Client()
    : fd(-1), registered(false), authenticated(false), nickSet(false), userSet(false), realname("")
{
    Logger::debug(LOG_CLIENT_CREATED);
}

// Destructor
Client::~Client() {
    if (fd >= 0) {
        close(fd);
        Logger::info(LOG_CLIENT_DISCONNECTED(fd));
    }
}

// Getters
int Client::getFd() const { return fd; }
std::string Client::getIPAddress() const { return IPAddress; }
std::string Client::getNickname() const { return nickname; }
std::string Client::getUsername() const { return username; }
std::string Client::getHostname() const { return hostname; }
std::string Client::getRealname() const { return realname; }
bool Client::isRegistered() const { return registered; }
bool Client::isAuthenticated() const { return authenticated; }
bool Client::isNickSet() const { return nickSet; }
bool Client::isUserSet() const { return userSet; }
std::string& Client::getCommandBuffer() { return commandBuffer; }

// Setters
void Client::setFd(int fd) { this->fd = fd; }
void Client::setIPAddress(const std::string& ipAddress) { this->IPAddress = ipAddress; }
void Client::setNickname(const std::string& nickname) {
    this->nickname = nickname;
    nickSet = !nickname.empty();
    Logger::debug(LOG_NICK_SET(nickname));
}
void Client::setUsername(const std::string& username) {
    this->username = username;
    userSet = !username.empty();
    Logger::debug(LOG_USERNAME_SET(username));
}
void Client::setHostname(const std::string& hostname) {
    this->hostname = hostname;
    Logger::debug(LOG_HOSTNAME_SET(hostname));
}

void Client::setRealname(const std::string& realname) {
    this->realname = realname;
    Logger::debug("Realname set to: " + realname);
}

void Client::setAuthenticated(bool status) {
    authenticated = status;
    Logger::debug(LOG_AUTH_STATUS(status));
}
void Client::setRegistered(bool status) {
    registered = status;
    Logger::debug(LOG_REG_STATUS(status));
}
void Client::setNickSet(bool status) { nickSet = status; }
void Client::setUserSet(bool status) { userSet = status; }

// Command Handling
void Client::appendToCommandBuffer(const std::string& data) {
    commandBuffer += data;
    Logger::debug("Appended " + Utils::intToString(data.length()) + " bytes to command buffer for fd " + Utils::intToString(fd));
}

// Reply Formatting
std::string Client::formatReply(const std::string& reply) {
    std::string formatted = reply;
    if (formatted.rfind(CRLF) != formatted.length() - 2) {
        formatted += CRLF;
    }
    if (formatted.length() > MAX_MESSAGE_LENGTH) {
        Logger::warning(LOG_SEND_TRUNCATED(fd));
        formatted = formatted.substr(0, MAX_MESSAGE_BODY) + CRLF;
    }
    return formatted;
}

void Client::handleSendResult(ssize_t bytesSent, const std::string& formattedReply)
{
    if (bytesSent < 0)
    {
        if (errno == EPIPE || errno == ECONNRESET)
        {
            Logger::debug("Peer already closed fd " + Utils::intToString(fd));
        }
        else
        {
            Logger::error(LOG_SEND_FAILED(fd, strerror(errno)));
        }
        return;
    }

    if (static_cast<size_t>(bytesSent) < formattedReply.length())
    {
        Logger::warning(LOG_PARTIAL_SEND(fd, bytesSent, formattedReply.length()));
    }
    else
    {
        Logger::debug("Sent " + Utils::intToString(bytesSent) +
                      " bytes to fd " + Utils::intToString(fd) + ": " + formattedReply);
    }
}

void Client::sendReply(const std::string& reply) {
    std::string formattedReply = formatReply(reply);
    ssize_t bytesSent = send(fd, formattedReply.c_str(), formattedReply.length(), 0);
    handleSendResult(bytesSent, formattedReply);
}

void Client::sendWelcomeHowTo(int fd)
{
    const char *lines[] = {
        ":ircserv 001 * :Welcome!  Please register in this exact order:\r\n",
        ":ircserv 002 * :  PASS <server-password>\r\n",
        ":ircserv 003 * :  NICK <nickname>\r\n",
        ":ircserv 004 * :  USER <user> 0 * :<real name>\r\n",
        ":ircserv 005 * :Then /JOIN channels and chat.  Commands must be UPPERCASE.\r\n",
        NULL
    };
    for (const char **p = lines; *p; ++p)
        send(fd, *p, strlen(*p), MSG_NOSIGNAL);
}
