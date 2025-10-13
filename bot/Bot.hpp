#ifndef BOT_HPP
#define BOT_HPP

#include <string>
#include <map>
#include <vector>
#include "PlayerStats.hpp"
#include "Room.hpp"

class Bot {
private:
    std::string host;
    int port;
    std::string password;
    int sock;
    std::map<std::string, PlayerStats> stats;
    // multiplayer rooms
    std::map<std::string, Room> rooms; // key: room name
    std::map<std::string, std::string> playerRoom; // nick -> room name
    void send_all(const std::string &msg);
    void send_privmsg(const std::string &target, const std::string &msg);
    void handleLine(const std::string &line);
    void send_scoreboard(const std::string &player);
    static std::string choose_rps();
    static int compare_rps(const std::string &player, const std::string &bot);
    // helpers to split handleLine
    bool isPing(const std::string &line, std::string &token) const;
    bool extractPrivmsg(const std::string &line, std::string &sender, std::string &target, std::string &message) const;
    bool isScoreOrStats(const std::string &lower) const;
    bool parsePlayerMove(const std::string &message, std::string &playerTok) const; // returns normalized "rock|paper|cisor" or false
    void handleScoreRequest(const std::string &sender);
    void handleMoveRound(const std::string &sender, const std::string &playerTok);
    // multiplayer helpers
    bool parseCommand(const std::string &msg, std::vector<std::string> &out) const; // split by spaces, lowercased
    void handleMultiplayerCommand(const std::string &sender, const std::vector<std::string> &cmd);
    void handleMultiplayerMove(const std::string &sender, const std::string &move);
    void notifyRoomBothChose(Room &room);
    void send_room_scoreboard(const std::string &sender);
public:
    Bot(const std::string &host, int port, const std::string &password);
    ~Bot();
    int run();
};

#endif
