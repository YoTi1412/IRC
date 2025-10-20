#include "Room.hpp"

Room::Room() : player1Wins(0), player2Wins(0), roundTies(0), roundsPlayed(0), setsWonP1(0), setsWonP2(0)
{

}

Room::Room(const std::string &nm) : name(nm), player1Wins(0), player2Wins(0), roundTies(0), roundsPlayed(0), setsWonP1(0), setsWonP2(0)
{

}

const std::string &Room::getName() const
{
    return name;
}

bool Room::isEmpty() const
{
    return player1Nick.empty() && player2Nick.empty();
}

bool Room::hasPlayer(const std::string &nick) const
{
    return nick == player1Nick || nick == player2Nick;
}

bool Room::addPlayer(const std::string &nick)
{
    if (player1Nick.empty())
    {
        player1Nick = nick; return true;
    }
    if (player2Nick.empty() && nick != player1Nick)
    {
        player2Nick = nick; return true;
    }
    return false;
}

void Room::removePlayer(const std::string &nick)
{
    if (nick == player1Nick)
    {
        player1Nick.clear();
        player1Choice.clear();
    }
    if (nick == player2Nick)
    {
        player2Nick.clear();
        player2Choice.clear();
    }
}

bool Room::isReady() const
{
    return !player1Nick.empty() && !player2Nick.empty();
}

bool Room::setPlayerChoice(const std::string &nick, const std::string &choice)
{
    if (nick == player1Nick)
    {
        player1Choice = choice;
        return true;
    }
    if (nick == player2Nick)
    {
        player2Choice = choice;
        return true;
    }
    return false;
}

bool Room::bothChose() const
{
    return !player1Choice.empty() && !player2Choice.empty();
}

void Room::clearChoices()
{
    player1Choice.clear();
    player2Choice.clear();
}

int Room::getRoundsPlayed() const
{
    return roundsPlayed;
}
int Room::getPlayer1Wins() const
{
    return player1Wins;
}
int Room::getPlayer2Wins() const
{
    return player2Wins;
}
int Room::getRoundTies() const
{
    return roundTies;
}
const std::string &Room::getPlayer1Nick() const
{
    return player1Nick;
}
const std::string &Room::getPlayer2Nick() const
{
    return player2Nick;
}
const std::string &Room::getPlayer1Choice() const
{
    return player1Choice;
}
const std::string &Room::getPlayer2Choice() const
{
    return player2Choice;
}

void Room::applyOutcome(int outcome)
{
    if (outcome > 0)
        ++player1Wins;
    else if (outcome < 0)
        ++player2Wins;
    else
        ++roundTies;
    ++roundsPlayed;
}

bool Room::isOver() const
{
    return roundsPlayed >= 5;
}

int Room::getPlayer1SetsWon() const
{
    return setsWonP1;
}

int Room::getPlayer2SetsWon() const
{
    return setsWonP2;
}

void Room::addPlayer1SetWin()
{
    ++setsWonP1;
}

void Room::addPlayer2SetWin()
{
    ++setsWonP2;
}

void Room::resetRoundCounters()
{
    player1Wins = 0;
    player2Wins = 0;
    roundTies = 0;
    roundsPlayed = 0;
    player1Choice.clear();
    player2Choice.clear();
}
