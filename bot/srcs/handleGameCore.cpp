#include "../includes/Bot.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdlib>




std::string Bot::choose_rps()
{
    int randomIndex = std::rand() % 3;
    if (randomIndex == 0)
        return "rock";
    if (randomIndex == 1)
        return "paper";
    return "cisor";
}







void Bot::send_scoreboard(const std::string &player)
{
    PlayerStats &playerStats = stats[player];
    std::ostringstream scoreboard;
    scoreboard << "+----------------------+-----+-----+-----+------+-----+\r\n";
    scoreboard << "| Player               |  W  |  L  |  T  | Sets |BotS |\r\n";
    scoreboard << "+----------------------+-----+-----+-----+------+-----+\r\n";
    std::ostringstream nameColumn;
    nameColumn << std::left << std::setw(20) << player;
    scoreboard << "| " << nameColumn.str() << " | "
        << std::setw(3) << playerStats.getWins() << " | "
        << std::setw(3) << playerStats.getLosses() << " | "
        << std::setw(3) << playerStats.getTies() << " | "
        << std::setw(4) << playerStats.getSetsWon() << " | "
        << std::setw(3) << playerStats.getBotSetsWon() << " |\r\n";
    scoreboard << "+----------------------+-----+-----+-----+------+-----+\r\n";

    std::istringstream scoreboardLines(scoreboard.str());
    std::string line;
    while (std::getline(scoreboardLines, line))
    {
        if (!line.empty() && line[line.size()-1] == '\r')
            line.erase(line.size()-1);
        send_privmsg(player, line);
    }
}



bool Bot::parsePlayerMove(const std::string &message, std::string &playerTok) const
{
    std::istringstream inputStream(message);
    std::vector<std::string> words;
    std::string token;
    while (inputStream >> token)
    {
        words.push_back(token);
    }
    if (words.empty())
    {
        playerTok.clear();
        return false;
    }

    if (words[0] == "cisor")
    {
        if (words.size() < 2)
        {
            playerTok.clear();
            return false;
        }
        std::string candidate = words[1];
        if (!isValidMoveToken(candidate))
        {
            playerTok.clear();
            return false;
        }
        if (candidate == "scissors")
            candidate = "cisor";
        playerTok = candidate;
        return true;
    }

    for (size_t index = 0; index < words.size(); ++index)
    {
        std::string candidate = words[index];
        if (isValidMoveToken(candidate))
        {
            if (candidate == "scissors")
                candidate = "cisor";
            playerTok = candidate;
            return true;
        }
    }
    playerTok.clear();
    return false;
}

void Bot::handleScoreRequest(const std::string &player)
{
    send_scoreboard(player);
}

void Bot::handleMoveRound(const std::string &sender, const std::string &playerTok)
{
    std::string botMove = choose_rps();
    int outcome = compare_rps(playerTok, botMove);

    PlayerStats &playerStats = stats[sender];
    if (outcome == 1)
        playerStats.addWin();
    else if (outcome == -1)
        playerStats.addLoss();
    else playerStats.addTie();
    playerStats.incrementPlayed();

    std::ostringstream resultMessage;
    resultMessage << "I choose " << botMove << "; you played " << playerTok;
    if (outcome == 1)
        resultMessage << " -- you win";
    else if (outcome == -1)
        resultMessage << " -- you lose";
    else
        resultMessage << " -- tie";
    send_privmsg(sender, resultMessage.str());

    if (playerStats.getPlayed() >= 5)
    {
        if (playerStats.getWins() > playerStats.getLosses())
            playerStats.addSetWon();
        else if (playerStats.getLosses() > playerStats.getWins())
            playerStats.addBotSetWon();
        send_scoreboard(sender);
        playerStats.resetSetCounters();
    }
}

int Bot::compare_rps(const std::string &player, const std::string &bot)
{
    if (player == bot)
        return 0;
    if (player == "rock")
    {
        if (bot == "cisor")
            return 1;
        else
            return -1;
    }
    if (player == "paper")
    {
        if (bot == "rock")
            return 1;
        else
            return -1;
    }
    if (player == "cisor")
    {
        if (bot == "paper")
            return 1;
        else
            return -1;
    }
    return -2;
}

bool Bot::isScore(const std::string &lower) const
{
    if (lower.find("score") != std::string::npos)
        return true;
    return false;
}