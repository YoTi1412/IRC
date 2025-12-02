#ifndef BOT_HPP
#define BOT_HPP

#include <string>
#include <map>
#include <vector>
#include "PlayerStats.hpp"
#include "Room.hpp"

#include "Includes.hpp"

class Bot {
private:
    std::string host;
    int port;
    std::string password;
    int sock;
    std::map<std::string, PlayerStats> stats;
    std::map<std::string, Room> rooms;
    std::map<std::string, std::string> playerRoom;

    void auth(const std::string &password);
    void mainLoop();
    int initSocket(struct sockaddr_in &srv);

    void send_all(const std::string &msg);
    void send_privmsg(const std::string &target, const std::string &msg);
    void send_scoreboard(const std::string &player);
    void handleScoreRequest(const std::string &sender);
    void notifyRoomNotEnoughPlayers(Room &room);
    void notifyOtherPlayerJoined(Room &room, const std::string &player);
    void sendRoomUsage(const std::string &player);
    void send_room_scoreboard(const std::string &sender);

    void handleLine(const std::string &line);
    bool extractPrivmsg(const std::string &line, std::string &sender, std::string &target, std::string &message) const;
    bool isScore(const std::string &lower) const;
    bool parseCommand(const std::string &msg, std::vector<std::string> &out) const;
    void notifyRoomBothChose(Room &room);

    int compare_rps(const std::string &player, const std::string &bot);
    bool parsePlayerMove(const std::string &message, std::string &playerTok) const;
    void handleMoveRound(const std::string &sender, const std::string &playerTok);
    void handleHelpCommand(const std::string &sender);
    static std::string choose_rps();

    void handleCreateRoom(const std::string &player, const std::vector<std::string> &command);
    void handleJoinRoom(const std::string &player, const std::vector<std::string> &command);
    void handleRoomStatus(const std::string &player);
    void handleLeaveRoom(const std::string &player);
    void handleMultiplayerCommand(const std::string &sender, const std::vector<std::string> &cmd);
    void leaveCurrentRoomIfAny(const std::string &player, const std::string &exceptRoom);
    void handleMultiplayerMove(const std::string &sender, const std::string &move);

public:
    Bot(const std::string &host, int port, const std::string &password);
    ~Bot();
    int run();
};

std::string toLowerStr(const std::string &s);
std::string trimTrailingPunct(const std::string &s);
bool isValidMoveToken(const std::string &t);
void trim_crlf(std::string &s);
void displayBanner();

#endif
