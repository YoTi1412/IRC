#include "Utils.hpp"
#include "Server.hpp"

bool Utils::isValidPort(const char* portStr) {
    for (size_t i = 0; portStr[i]; i++)
        if (!isdigit(portStr[i]))
            return false;

    long portNum = std::strtol(portStr, NULL, 10);

    if (portNum < 1024 || portNum > 65535)
        return false;
    return true;
}

bool Utils::isValidPassword(const std::string& password) {
    if (password.empty()) return false;
    for (size_t i = 0; i < password.size(); i++) {
        unsigned char c = static_cast<unsigned char>(password[i]);
        if (!std::isprint(c) || std::isspace(c)) {
            return false;
        }
    }
    return true;
}

std::string Utils::getFormattedTime() {
    time_t now = time(NULL);
    struct tm* timeinfo = localtime(&now);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", timeinfo);
    return std::string(buf);
}

void Utils::setupSignalHandler() {
    signal(SIGINT, Server::sigHandler);
    signal(SIGQUIT, Server::sigHandler);
}

std::string Utils::intToString(int value) {
    std::stringstream ss;
    ss << value;
    return ss.str();
}

int Utils::setnonblocking(int client_fd) {
    int flags = fcntl(client_fd, F_GETFL, 0);
    if (flags < 0)
        return -1;
    if (fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) < 0)
        return -1;
    return 0;
}

std::list<std::string> Utils::splitCommand(const std::string& buffer) {
    std::list<std::string> cmdList;
    size_t pos = buffer.find("\r\n");
    if (pos != std::string::npos) {
        std::string cmd = buffer.substr(0, pos);
        if (!cmd.empty()) {
            std::istringstream iss(cmd);
            std::string token;
            while (iss >> token) {
                cmdList.push_back(token);
            }
        }
    }
    return cmdList;
}

std::string Utils::toLower(const std::string& str) {
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
    return lowerStr;
}


std::list<std::string> Utils::split(const std::string& str, char delim) {
    std::list<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    while (std::getline(tokenStream, token, delim)) {
        tokens.push_back(token);
    }
    return tokens;
}