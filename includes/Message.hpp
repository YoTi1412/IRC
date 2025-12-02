#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

class Message {
private:
    std::string prefix;
    std::string command;
    std::vector<std::string> params;

public:
    Message() { }
    Message(const std::string &cmd) : command(cmd) { }

    const std::string &getPrefix() const { return prefix; }
    const std::string &getCommand() const { return command; }
    const std::vector<std::string> &getParams() const { return params; }

    bool hasPrefix() const { return !prefix.empty(); }
    bool hasParams() const { return !params.empty(); }

    void setPrefix(const std::string &p) { prefix = p; }
    void setCommand(const std::string &c) { command = c; }
    void addParam(const std::string &p) { params.push_back(p); }
    void clear() { prefix.clear(); command.clear(); params.clear(); }

    // Parse a raw IRC line into a Message object.
    // Handles optional prefix, command, and parameters, including trailing param prefixed by ':'.
    static Message parseLine(const std::string &line) {
        Message msg;
        std::string s = line;

        // remove trailing CR if present
        if (!s.empty() && s.size() >= 1 && s[s.size() - 1] == '\r') {
            s.erase(s.size() - 1, 1);
        }

        size_t pos = 0;

        // optional prefix
        if (!s.empty() && s[0] == ':') {
            size_t sp = s.find(' ');
            if (sp != std::string::npos) {
                if (sp > 1)
                    msg.prefix = s.substr(1, sp - 1);
                else
                    msg.prefix = std::string();
                pos = sp + 1;
            } else {
                // only prefix present
                if (s.size() > 1)
                    msg.prefix = s.substr(1);
                return msg;
            }
        }

        // skip spaces
        while (pos < s.size() && s[pos] == ' ') ++pos;
        if (pos >= s.size()) return msg;

        // command
        size_t next = s.find(' ', pos);
        if (next == std::string::npos) {
            msg.command = s.substr(pos);
            return msg;
        }
        msg.command = s.substr(pos, next - pos);
        pos = next + 1;

        // parameters
        while (pos < s.size()) {
            if (s[pos] == ' ') { ++pos; continue; }
            if (s[pos] == ':') {
                // trailing parameter - take rest of the line
                msg.params.push_back(s.substr(pos + 1));
                break;
            }
            size_t sp = s.find(' ', pos);
            if (sp == std::string::npos) {
                msg.params.push_back(s.substr(pos));
                break;
            } else {
                msg.params.push_back(s.substr(pos, sp - pos));
                pos = sp + 1;
            }
        }

        return msg;
    }

    // Build a raw IRC line from this Message.
    // Uses CRLF externally; this function returns the textual line without adding CRLF.
    std::string toRaw() const {
        std::ostringstream oss;
        if (!prefix.empty()) {
            oss << ':' << prefix << ' ';
        }

        oss << command;

        for (size_t i = 0; i < params.size(); ++i) {
            const std::string &p = params[i];
            // If a parameter contains spaces or starts with ':' it must be sent as a trailing param.
            bool needsTrailing = (p.find(' ') != std::string::npos) || (!p.empty() && p[0] == ':');
            if (needsTrailing) {
                oss << ' ' << ':' << p;
                break;
            } else {
                oss << ' ' << p;
            }
        }

        return oss.str();
    }
};

#endif