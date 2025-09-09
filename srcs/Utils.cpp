#include "Utils.hpp"
#include "Server.hpp"

bool Utils::isValidPort(const char* portStr)
{
    for (size_t i = 0; portStr[i]; i++)
        if (!isdigit(portStr[i]))
            return false;

    long portNum = std::strtol(portStr, NULL, 10);

    if (portNum < 1024 || portNum > 65535)
        return false;
    return true;
}

bool Utils::isValidPassword(const std::string& password)
{
    if (password.empty()) return false;
    for (size_t i = 0; i < password.size(); i++)
    {
        unsigned char c = static_cast<unsigned char>(password[i]);
        if (!std::isprint(c) || std::isspace(c))
        {
            return false;
        }
    }
    return true;
}

std::string Utils::getFormattedTime()
{
    time_t now = time(0);
    char buf[80];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    return std::string(buf);
}

void Utils::setupSignalHandler()
{
    signal(SIGINT, Server::sigHandler);
    signal(SIGQUIT, Server::sigHandler);
}

std::string Utils::intToString(int value) {
    std::stringstream ss;
    ss << value;
    return ss.str();
}


// uint16_t ft_htons(uint16_t hostshort) {
//     unsigned char test[2] = {1, 0};  // 0x0001 in memory
//     // If first byte is 1, machine is little-endian
//     if (*(short*)test == 1) {
//         // Little-endian: swap the bytes
//         return (hostshort >> 8) | (hostshort << 8);
//     } else {
//         // Big-endian: no need to swap
//         return hostshort;
//     }
// }


// std::list<std::string> Utils::splitString(std::string& cmd) { return std::list<std::string>(); }
// void Utils::sendError(int fd, const std::string& message) {}
// void Utils::sendReply(int fd, const std::string& message) {}
// int Utils::setnonblocking(int client_fd) { return 0; }
// bool Utils::isNumber(const std::string& str) { return false; }
