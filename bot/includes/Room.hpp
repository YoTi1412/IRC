#ifndef ROOM_HPP
#define ROOM_HPP

#include <string>

class Room {
private:
    std::string name;
    std::string player1Nick;
    std::string player2Nick;
    std::string player1Choice;
    std::string player2Choice;
    int player1Wins;
    int player2Wins;
    int roundTies;
    int roundsPlayed;
    int setsWonP1;
    int setsWonP2;
public:
    Room();
    ~Room();
    Room(const std::string &name);
    const std::string &getName() const;
    bool isEmpty() const;
    bool hasPlayer(const std::string &nick) const;
    bool addPlayer(const std::string &nick);
    void removePlayer(const std::string &nick);
    bool isReady() const;
    bool setPlayerChoice(const std::string &nick, const std::string &choice);
    bool bothChose() const;
    void clearChoices();
    int getRoundsPlayed() const;
    int getPlayer1Wins() const;
    int getPlayer2Wins() const;
    int getRoundTies() const;
    const std::string &getPlayer1Nick() const;
    const std::string &getPlayer2Nick() const;
    const std::string &getPlayer1Choice() const;
    const std::string &getPlayer2Choice() const;
    void applyOutcome(int outcome);
    bool isOver() const;

    int getPlayer1SetsWon() const;
    int getPlayer2SetsWon() const;
    void addPlayer1SetWin();
    void addPlayer2SetWin();
    void resetRoundCounters();
};

#endif
