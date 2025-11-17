#ifndef PLAYERSTATS_HPP
#define PLAYERSTATS_HPP

#include <string>

class PlayerStats {
private:
    int wins;
    int losses;
    int ties;
    int played;
    int sets_won;
    int bot_sets_won;
public:
    PlayerStats();
    ~PlayerStats();
    void addWin();
    void addLoss();
    void addTie();
    void incrementPlayed();
    int getWins() const;
    int getLosses() const;
    int getTies() const;
    int getPlayed() const;
    int getSetsWon() const;
    int getBotSetsWon() const;
    void resetSetCounters();
    void addSetWon();
    void addBotSetWon();
    std::string toSummary() const;
};

#endif
