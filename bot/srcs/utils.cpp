#include "../includes/Bot.hpp"

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
