#include "Includes.hpp"
#include <stdexcept>

// Constructor
Client::Client()
    : fd(-1), registered(false), authenticated(false), nickSet(false), userSet(false), realname(""), greeted(false)
{
    Logger::info(LOG_CLIENT_CREATED);
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
    Logger::info(LOG_NICK_SET(nickname));
}
void Client::setUsername(const std::string& username) {
    this->username = username;
    userSet = !username.empty();
    Logger::info(LOG_USERNAME_SET(username));
}
void Client::setHostname(const std::string& hostname) {
    this->hostname = hostname;
    Logger::info(LOG_HOSTNAME_SET(hostname));
}

void Client::setRealname(const std::string& realname) {
    this->realname = realname;
    Logger::info("Realname set to: " + realname);
}

void Client::setAuthenticated(bool status) {
    authenticated = status;
    Logger::info(LOG_AUTH_STATUS(status));
}
void Client::setRegistered(bool status) {
    registered = status;
    Logger::info(LOG_REG_STATUS(status));
}
void Client::setNickSet(bool status) { nickSet = status; }
void Client::setUserSet(bool status) { userSet = status; }

bool Client::isGreeted() const { return greeted; }
void Client::setGreeted(bool greeted) { this->greeted = greeted; }

// Command Handling
void Client::appendToCommandBuffer(const std::string& data) {
    commandBuffer += data;
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
            Logger::warning("Peer already closed fd " + Utils::intToString(fd));
        }
        else
        {
            throw std::invalid_argument(LOG_SEND_FAILED(fd, strerror(errno)));
        }
        return;
    }

    if (static_cast<size_t>(bytesSent) < formattedReply.length())
    {
        Logger::warning(LOG_PARTIAL_SEND(fd, bytesSent, formattedReply.length()));
    }
    else
    {
        // Successful send — avoid verbose logging here to prevent leaking message content
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
        ":ircserv NOTICE * :Welcome! Please register in this exact order:\r\n",
        ":ircserv NOTICE * :  PASS <server-password>\r\n",
        ":ircserv NOTICE * :  NICK <nickname>\r\n",
        ":ircserv NOTICE * :  USER <user> 0 * :<real name>\r\n",
        ":ircserv NOTICE * :Then #JOIN channels and chat. Commands must be UPPERCASE.\r\n",
        NULL
    };
    for (const char **p = lines; *p; ++p)
        send(fd, *p, strlen(*p), MSG_NOSIGNAL);
}
