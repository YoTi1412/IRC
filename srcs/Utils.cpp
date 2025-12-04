#include "Includes.hpp"
#include <iostream>

void Utils::displayBanner() {
    std::cout << "                                             \n";
    std::cout << " Welcome to:                                         \n";
    std::cout << "  _                                         \n";
    std::cout << " (_)                                        \n";
    std::cout << "  _ _ __ ___   ___  ___ _ ____   _____ _ __ \n";
    std::cout << " | | '__/ __| / __|/ _ \\ '__\\ \\ / / _ \\ '__|\n";
    std::cout << " | | | | (__  \\__ \\  __/ |   \\ V /  __/ |   \n";
    std::cout << " |_|_|  \\___| |___/\\___|_|    \\_/ \\___|_|   \n";
    std::cout << "                                             \n";
    std::cout << "                                             \n";
    std::cout << " By: yrafai && mkobaa\n";
    std::cout << "\n";
}

std::string Utils::formatTime(time_t t) {
    struct tm* timeinfo = localtime(&t);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", timeinfo);
    return std::string(buf);
}

bool Utils::isValidPort(const char* portStr) {
    for (size_t i = 0; portStr[i]; i++)
        if (!isdigit(portStr[i]))
            return false;

    long portNum = std::strtol(portStr, NULL, 10);

    if (portNum < MIN_PORT || portNum > MAX_PORT)
        return false;
    return true;
}

bool Utils::isValidPassword(const std::string& password) {
    if (password.empty())
        return false;
    for (size_t i = 0; i < password.size(); i++) {
        unsigned char c = static_cast<unsigned char>(password[i]);
        if (!std::isprint(c) || std::isspace(c)) {
            return false;
        }
    }
    return true;
}

void Utils::setupSignalHandler() {
    signal(SIGINT, Server::sigHandler);
    signal(SIGQUIT, Server::sigHandler);
    signal(SIGPIPE, SIG_IGN);
}

std::string Utils::intToString(int value) {
    std::stringstream ss;
    ss << value;
    return ss.str();
}

int Utils::setnonblocking(int fd) {
    return fcntl(fd, F_SETFL, O_NONBLOCK);
}

std::list<std::string> Utils::splitCommand(const std::string& buffer) {
    std::list<std::string> cmdList;
    size_t pos = buffer.find(CRLF);
    if (pos != std::string::npos) {
        std::string cmd = buffer.substr(0, pos);
        if (!cmd.empty()) {
            size_t colonPos = cmd.find(" :");
            if (colonPos != std::string::npos) {
                std::string beforeColon = cmd.substr(0, colonPos);
                std::istringstream iss(beforeColon);
                std::string token;
                while (iss >> token) {
                    if (!token.empty()) cmdList.push_back(token);
                }
                std::string trailing = cmd.substr(colonPos + 2);
                if (!trailing.empty()) cmdList.push_back(trailing);
            } else {
                std::istringstream iss(cmd);
                std::string token;
                while (iss >> token) {
                    if (!token.empty()) cmdList.push_back(token);
                }
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
