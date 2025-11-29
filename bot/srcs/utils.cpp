#include "../includes/Bot.hpp"
#include <iostream>

std::string toLowerStr(const std::string &s)
{
    std::string out = s;
    for (size_t i = 0; i < out.size(); ++i)
        out[i] = (char)std::tolower((unsigned char)out[i]);
    return out;
}

std::string trimTrailingPunct(const std::string &s)
{
    if (s.empty()) return s;
    char last = s[s.size() - 1];
    if (last == ':' || last == ',')
        return s.substr(0, s.size() - 1);
    return s;
}

bool isValidMoveToken(const std::string &t)
{
    if (t == "rock")
        return true;
    else if (t == "paper")
        return true;
    else if (t == "cisor" || t == "scissors")
        return true;
    else
        return false;
}

void trim_crlf(std::string &s)
{
    while (!s.empty())
    {
        char c = s[s.size()-1];
        if (c == '\r' || c == '\n')
            s.erase(s.size()-1);
        else
            break;
    }
}

void displayBanner() {

    std::cout << "  _            _           _   \n";
    std::cout << " (_)          | |         | |  \n";
    std::cout << "  _ _ __ ___  | |__   ___ | |_ \n";
    std::cout << " | | '__/ __| | '_ \\ / _ \\| __|\n";
    std::cout << " | | | | (__  | |_) | (_) | |_ \n";
    std::cout << " |_|_|  \\___| |_.__/ \\___/ \\__|\n";
    std::cout << "                                \n";
    std::cout << " v0.0.0.0.0.0.0.0.0.0.0.0.0.0.1\n";
}
