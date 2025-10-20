#include "Bot.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdlib>




std::string Bot::choose_rps()
{
    int v = std::rand() % 3;
    if (v == 0)
        return "rock";
    if (v == 1)
        return "paper";
    return "cisor";
}







void Bot::send_scoreboard(const std::string &player)
{
    PlayerStats &st = stats[player];
    std::ostringstream oss;
    oss << "+----------------------+-----+-----+-----+------+-----+\r\n";
    oss << "| Player               |  W  |  L  |  T  | Sets |BotS |\r\n";
    oss << "+----------------------+-----+-----+-----+------+-----+\r\n";
    std::ostringstream namecol;
    namecol << std::left << std::setw(20) << player;
    oss << "| " << namecol.str() << " | "
        << std::setw(3) << st.getWins() << " | "
        << std::setw(3) << st.getLosses() << " | "
        << std::setw(3) << st.getTies() << " | "
        << std::setw(4) << st.getSetsWon() << " | "
        << std::setw(3) << st.getBotSetsWon() << " |\r\n";
    oss << "+----------------------+-----+-----+-----+------+-----+\r\n";

    std::istringstream lines(oss.str());
    std::string line;
    while (std::getline(lines, line))
    {
        if (!line.empty() && line[line.size()-1] == '\r')
            line.erase(line.size()-1);
        send_privmsg(player, line);
    }
}



bool Bot::parsePlayerMove(const std::string &message, std::string &playerTok) const
{
    std::istringstream iss(message);
    std::vector<std::string> tokens;
    std::string tok;
    while (iss >> tok)
    {
        tokens.push_back(tok);
    }
    if (tokens.empty())
    {
        playerTok.clear();
        return false;
    }

    if (tokens[0] == "cisor")
    {
        if (tokens.size() < 2)
        {
            playerTok.clear();
            return false;
        }
        std::string cand = tokens[1];
        if (!isValidMoveToken(cand))
        {
            playerTok.clear();
            return false;
        }
        if (cand == "scissors")
            cand = "cisor";
        playerTok = cand;
        return true;
    }

    for (size_t i = 0; i < tokens.size(); ++i)
    {
        std::string cand = tokens[i];
        if (isValidMoveToken(cand))
        {
            if (cand == "scissors")
                cand = "cisor";
            playerTok = cand;
            return true;
        }
    }
    playerTok.clear();
    return false;
}

void Bot::handleScoreRequest(const std::string &sender)
{
    send_scoreboard(sender);
}

void Bot::handleMoveRound(const std::string &sender, const std::string &playerTok)
{
    std::string botTok = choose_rps();
    int outcome = compare_rps(playerTok, botTok);

    PlayerStats &st = stats[sender];
    if (outcome == 1)
        st.addWin();
    else if (outcome == -1)
        st.addLoss(); else st.addTie();
    st.incrementPlayed();

    std::ostringstream res;
    res << "I choose " << botTok << "; you played " << playerTok;
    if (outcome == 1)
        res << " -- you win";
    else if (outcome == -1)
        res << " -- you lose";
    else
        res << " -- tie";
    send_privmsg(sender, res.str());

    if (st.getPlayed() >= 5)
    {
        if (st.getWins() > st.getLosses())
            st.addSetWon();
        else if (st.getLosses() > st.getWins())
            st.addBotSetWon();
        send_scoreboard(sender);
        st.resetSetCounters();
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