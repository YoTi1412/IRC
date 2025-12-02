#include "../includes/PlayerStats.hpp"
#include <sstream>

PlayerStats::PlayerStats(): wins(0), losses(0), ties(0), played(0), sets_won(0), bot_sets_won(0)
{

}

PlayerStats::~PlayerStats()
{

}

void PlayerStats::addWin()
{
    ++wins;
}

void PlayerStats::addLoss()
{
    ++losses;
}

void PlayerStats::addTie()
{
    ++ties;
}
void PlayerStats::incrementPlayed()
{
    ++played;
}

int PlayerStats::getWins() const
{
    return wins;
}
int PlayerStats::getLosses() const
{
    return losses;
}
int PlayerStats::getTies() const
{
    return ties;
}
int PlayerStats::getPlayed() const
{
    return played;
}
int PlayerStats::getSetsWon() const
{
    return sets_won;
}
int PlayerStats::getBotSetsWon() const
{
    return bot_sets_won;
}

void PlayerStats::resetSetCounters()
{
    wins = 0; losses = 0; ties = 0; played = 0;
}
void PlayerStats::addSetWon()
{
    ++sets_won;
}
void PlayerStats::addBotSetWon()
{
    ++bot_sets_won;
}

std::string PlayerStats::toSummary() const
{
    std::ostringstream oss;
    oss << "wins=" << wins << " losses=" << losses << " ties=" << ties;
    return oss.str();
}
