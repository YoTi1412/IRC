#pragma once
#include "Includes.hpp"

class Utils {
private:
    Utils();
    Utils(const Utils& other);
    Utils& operator=(const Utils& other);
    ~Utils();

public:
    static bool isValidPort(const char* portStr);
    static bool isValidPassword(const std::string& password);
    static void setupSignalHandler();
    static std::list<std::string> splitString(std::string& cmd);
    static void sendError(int fd, const std::string& message);
    static void sendReply(int fd, const std::string& message);
    static std::string getFormattedTime();
    static int setnonblocking(int client_fd);
    static bool isNumber(const std::string& str);
    static std::string intToString(int value);
};
