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
    static std::list<std::string> splitCommand(const std::string& buffer);
    static void sendError(int fd, const std::string& message);
    static void sendReply(int fd, const std::string& message);
    static std::string getFormattedTime();
    static int setnonblocking(int client_fd);
    static bool isNumber(const std::string& str);
    static std::string intToString(int value);
    static std::string toLower(const std::string& str);
    static std::list<std::string> split(const std::string& str, char delim);
    static std::string formatTime(time_t t);
};
