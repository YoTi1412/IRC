#include "Bot.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>

void Bot::handleMultiplayerCommand(const std::string &sender, const std::vector<std::string> &cmd)
{
    if (cmd.empty() || cmd[0] != "room")
        return;

    if (cmd.size() < 2)
    {
        send_privmsg(sender, "Usage: room <create|join|leave|status> [name]");
        return;
    }
    const std::string &sub = cmd[1];
    if (sub == "create")
    {
        if (cmd.size() < 3)
        {
            send_privmsg(sender, "Usage: room create <name>");
            return;
        }
        std::map<std::string, std::string>::iterator cur = playerRoom.find(sender);
        if (cur != playerRoom.end())
        {
            std::string oldRn = cur->second;
            Room &old = rooms[oldRn];
            // Notify the other player (if any)
            std::string other;
            if (old.getPlayer1Nick() == sender)
                other = old.getPlayer2Nick();
            else if (old.getPlayer2Nick() == sender)
                other = old.getPlayer1Nick();
            old.removePlayer(sender);
            playerRoom.erase(cur);
            send_privmsg(sender, "You left room '" + oldRn + "'.");
            if (!other.empty())
            {
                send_privmsg(other, sender + " left your room '" + oldRn + "'. There are not enough players in the room.");
            }
            if (old.isEmpty())
            {
                rooms.erase(oldRn);
            }
        }
        std::string rn = cmd[2];
        if (rooms.find(rn) != rooms.end())
        {
            send_privmsg(sender, "Room already exists.");
            return;
        }
        Room r(rn);
        r.addPlayer(sender);
        rooms[rn] = r;
        playerRoom[sender] = rn;
        send_privmsg(sender, "Room '" + rn + "' created. Waiting for an opponent (ask them to 'room join " + rn + "').");
    }
    else if (sub == "join")
    {
        if (cmd.size() < 3)
        {
            send_privmsg(sender, "Usage: room join <name>");
            return;
        }
        std::string rn = cmd[2];
        // If user is already in a different room, auto-leave it first
        std::map<std::string, std::string>::iterator cur = playerRoom.find(sender);
        if (cur != playerRoom.end() && cur->second != rn)
        {
            std::string oldRn = cur->second;
            Room &old = rooms[oldRn];
            // Notify the other player (if any)
            std::string other;
            if (old.getPlayer1Nick() == sender)
                other = old.getPlayer2Nick();
            else if (old.getPlayer2Nick() == sender)
                other = old.getPlayer1Nick();
            old.removePlayer(sender);
            playerRoom.erase(cur);
            send_privmsg(sender, "You left room '" + oldRn + "'.");
            if (!other.empty())
            {
                send_privmsg(other, sender + " left your room '" + oldRn + "'. There are not enough players in the room.");
            }
            if (old.isEmpty())
            {
                rooms.erase(oldRn);
            }
        }
        std::map<std::string, Room>::iterator it = rooms.find(rn);
        if (it == rooms.end())
        {
            send_privmsg(sender, "No such room.");
            return;
        }
        Room &r = it->second;
        if (r.hasPlayer(sender))
        {
            send_privmsg(sender, "You are already in this room.");
            return;
        }
        if (!r.addPlayer(sender))
        {
            send_privmsg(sender, "Room is full.");
            return;
        }
        playerRoom[sender] = rn;
        send_privmsg(sender, "Joined room '" + rn + "'. Both players DM 'play <rock|paper|cisor>' to play rounds.");
        if (!r.getPlayer1Nick().empty() && r.getPlayer1Nick() != sender)
        {
            send_privmsg(r.getPlayer1Nick(), sender + " joined your room '" + rn + "'");
        }
        if (!r.getPlayer2Nick().empty() && r.getPlayer2Nick() != sender)
        {
            send_privmsg(r.getPlayer2Nick(), sender + " joined your room '" + rn + "'");
        }
    }
    else if (sub == "leave")
    {
        std::map<std::string, std::string>::iterator pr = playerRoom.find(sender);
        if (pr == playerRoom.end())
        {
            send_privmsg(sender, "You're not in a room.");
            return;
        }
        std::string rn = pr->second;
        Room &r = rooms[rn];
        r.removePlayer(sender);
        playerRoom.erase(pr);
        send_privmsg(sender, "Left room '" + rn + "'.");
        // Notify remaining player, if any
        if (!r.getPlayer1Nick().empty())
        {
            send_privmsg(r.getPlayer1Nick(), "There are not enough players in the room.");
        }
        if (!r.getPlayer2Nick().empty())
        {
            send_privmsg(r.getPlayer2Nick(), "There are not enough players in the room.");
        }
        if (r.isEmpty())
        {
            rooms.erase(rn);
        }
    }
    else if (sub == "status")
    {
        std::map<std::string, std::string>::iterator pr = playerRoom.find(sender);
        if (pr == playerRoom.end())
        {
            send_privmsg(sender, "You're not in a room.");
            return;
        }
        Room &r = rooms[pr->second];
        if (!r.isReady())
        {
            send_privmsg(sender, "There are not enough players in the room.");
            return;
        }
        std::ostringstream ss;
    ss << "Room '" << r.getName() << "' players: [" << r.getPlayer1Nick() << ", " << r.getPlayer2Nick() << "] rounds: " << r.getRoundsPlayed()
       << "; score: " << r.getPlayer1Nick() << " " << r.getPlayer1Wins() << "-" << r.getPlayer2Wins() << " " << r.getPlayer2Nick()
       << "; sets: " << r.getPlayer1Nick() << "=" << r.getPlayer1SetsWon() << ", " << r.getPlayer2Nick() << "=" << r.getPlayer2SetsWon();
        send_privmsg(sender, ss.str());
    }
    else
    {
        send_privmsg(sender, "Unknown subcommand. Use: room <create|join|leave|status> [name]");
    }
}


void Bot::handleMultiplayerMove(const std::string &sender, const std::string &move)
{
    std::map<std::string, std::string>::iterator playerRoomIt = playerRoom.find(sender);
    if (playerRoomIt == playerRoom.end())
    {
        send_privmsg(sender, "Join a room first: room create <name> or room join <name>");
        return;
    }
    Room &r = rooms[playerRoomIt->second];
    if (!r.isReady())
    {
        send_privmsg(sender, "There are not enough players in the room.");
        return;
    }
    if (!isValidMoveToken(move))
    {
        send_privmsg(sender, "Invalid move. Use rock|paper|cisor.");
        return;
    }
    std::string norm = (move == "scissors") ? "cisor" : move;
    r.setPlayerChoice(sender, norm);
    send_privmsg(sender, "Move received. Waiting for the opponent...");
    if (r.bothChose())
    {
        notifyRoomBothChose(r);
    }
}

void Bot::notifyRoomBothChose(Room &room)
{
    if (!room.isReady())
    {
        // A player left or switched rooms; do not proceed.
        return;
    }
    int outcome = compare_rps(room.getPlayer1Choice(), room.getPlayer2Choice());
    room.applyOutcome(outcome);
    std::string msg;
    if (outcome > 0)
    msg = room.getPlayer1Nick() + " wins this round";
    else if (outcome < 0)
    msg = room.getPlayer2Nick() + " wins this round";
    else
        msg = "Round is a tie";
    std::ostringstream round;
    round << "Choices: " << room.getPlayer1Nick() << "=" << room.getPlayer1Choice() << ", " << room.getPlayer2Nick() << "=" << room.getPlayer2Choice() << "; " << msg << ". (" << room.getRoundsPlayed() << "/5)";
    if (!room.getPlayer1Nick().empty())
        send_privmsg(room.getPlayer1Nick(), round.str());
    if (!room.getPlayer2Nick().empty())
        send_privmsg(room.getPlayer2Nick(), round.str());
    room.clearChoices();

    if (room.isOver())
    {
        // Determine set winner, increment set counters, announce, then reset rounds (keep room open)
        if (room.getPlayer1Wins() > room.getPlayer2Wins())
        {
            room.addPlayer1SetWin();
        }
        else if (room.getPlayer2Wins() > room.getPlayer1Wins())
        {
            room.addPlayer2SetWin();
        }
        std::ostringstream fin;
        fin << "Set finished. Round score: " << room.getPlayer1Nick() << " " << room.getPlayer1Wins() << " - " << room.getPlayer2Wins() << " " << room.getPlayer2Nick() << 
               ", ties=" << room.getRoundTies() << "; Sets: " << room.getPlayer1Nick() << "=" << room.getPlayer1SetsWon() << ", " << room.getPlayer2Nick() << "=" << room.getPlayer2SetsWon();
        if (!room.getPlayer1Nick().empty())
            send_privmsg(room.getPlayer1Nick(), fin.str());
        if (!room.getPlayer2Nick().empty())
            send_privmsg(room.getPlayer2Nick(), fin.str());
        // Reset per-set counters for next set
        room.resetRoundCounters();
    }
}

void Bot::send_room_scoreboard(const std::string &sender)
{
    std::map<std::string, std::string>::iterator pr = playerRoom.find(sender);
    if (pr == playerRoom.end())
    {
        send_privmsg(sender, "You're not in a room. Use 'room status' or personal 'score'.");
        return;
    }
    Room &r = rooms[pr->second];

    if (!r.isReady())
    {
        send_privmsg(sender, "There are not enough players in the room.");
        return;
    }

    // Gather both players' stats (no structs; plain variables)
    std::string p1n = r.getPlayer1Nick();
    int p1w = r.getPlayer1Wins();
    int p1l = r.getPlayer2Wins();
    int p1t = r.getRoundTies();
    int p1g = r.getRoundsPlayed();
    int p1s = r.getPlayer1SetsWon();

    std::string p2n = r.getPlayer2Nick();
    int p2w = r.getPlayer2Wins();
    int p2l = r.getPlayer1Wins();
    int p2t = r.getRoundTies();
    int p2g = r.getRoundsPlayed();
    int p2s = r.getPlayer2SetsWon();

    // Determine rank: set wins desc, then wins desc, ties desc, name asc
    bool firstIsP1 = true;
    if (p2s > p1s)
    {
        firstIsP1 = false;
    }
    else if (p2s == p1s && p2w > p1w)
    {
        firstIsP1 = false;
    }
    else if (p2s == p1s && p2w == p1w)
    {
        if (p2t > p1t)
            firstIsP1 = false;
        else if (p2t == p1t && p2n < p1n)
            firstIsP1 = false;
    }

    std::ostringstream oss;
    oss << "+----------------------------------------------+-----+-----+-----+--------+------+\r\n";
    oss << "| Rank | Player               |  W  |  L  |  T  | Games  | Sets |\r\n";
    oss << "+----------------------------------------------+-----+-----+-----+--------+------+\r\n";
    for (int rank = 1; rank <= 2; ++rank)
    {
        std::string name; int w; int l; int t; int g; int s;
        if ((rank == 1 && firstIsP1) || (rank == 2 && !firstIsP1))
        {
            name = p1n; w = p1w; l = p1l; t = p1t; g = p1g; s = p1s;
        }
        else
        {
            name = p2n; w = p2w; l = p2l; t = p2t; g = p2g; s = p2s;
        }
        std::ostringstream namecol;
        namecol << std::left << std::setw(20) << name;
        oss << "|  " << rank << "   | " << namecol.str() << " | "
            << std::setw(3) << w << " | "
            << std::setw(3) << l << " | "
            << std::setw(3) << t << " | "
            << std::setw(6) << g << " | "
            << std::setw(4) << s << " |\r\n";
    }
    oss << "+----------------------------------------------+-----+-----+-----+--------+------+\r\n";

    std::istringstream lines(oss.str());
    std::string ln;
    while (std::getline(lines, ln))
    {
        if (!ln.empty() && ln[ln.size()-1] == '\r')
            ln.erase(ln.size()-1);
        if (!r.getPlayer1Nick().empty())
            send_privmsg(r.getPlayer1Nick(), ln);
        if (!r.getPlayer2Nick().empty() && r.getPlayer2Nick() != r.getPlayer1Nick())
            send_privmsg(r.getPlayer2Nick(), ln);
    }
}
