#include "Bot.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>

void Bot::handleMultiplayerCommand(const std::string &player, const std::vector<std::string> &command)
{
    if (command.empty() || command[0] != "room")
        return;

    if (command.size() < 2)
    {
        sendRoomUsage(player);
        return;
    }

    const std::string &subCommand = command[1];

    if (subCommand == "create")
        handleCreateRoom(player, command);
    else if (subCommand == "join")
        handleJoinRoom(player, command);
    else if (subCommand == "leave")
        handleLeaveRoom(player);
    else if (subCommand == "status")
        handleRoomStatus(player);
    else
        sendRoomUsage(player);
}


void Bot::sendRoomUsage(const std::string &player)
{
    send_privmsg(player, "Usage: room <create|join|leave|status> [name]");
}

void Bot::handleCreateRoom(const std::string &player, const std::vector<std::string> &command)
{
    if (command.size() < 3)
    {
        send_privmsg(player, "Usage: room create <name>");
        return;
    }

    leaveCurrentRoomIfAny(player, "");

    const std::string &roomName = command[2];
    if (rooms.find(roomName) != rooms.end())
    {
        send_privmsg(player, "Room already exists.");
        return;
    }

    Room newRoom(roomName);
    newRoom.addPlayer(player);
    rooms[roomName] = newRoom;
    playerRoom[player] = roomName;

    send_privmsg(player, "Room '" + roomName + "' created. Waiting for an opponent (ask them to 'room join " + roomName + "').");
}

void Bot::handleJoinRoom(const std::string &player, const std::vector<std::string> &command)
{
    if (command.size() < 3)
    {
        send_privmsg(player, "Usage: room join <name>");
        return;
    }

    const std::string &roomName = command[2];

    leaveCurrentRoomIfAny(player, roomName);

    std::map<std::string, Room>::iterator roomIterator = rooms.find(roomName);
    if (roomIterator == rooms.end())
    {
        send_privmsg(player, "No such room.");
        return;
    }

    Room &room = roomIterator->second;

    if (room.hasPlayer(player))
    {
        send_privmsg(player, "You are already in this room.");
        return;
    }

    if (!room.addPlayer(player))
    {
        send_privmsg(player, "Room is full.");
        return;
    }

    playerRoom[player] = roomName;

    send_privmsg(player, "Joined room '" + roomName + "'. Both players DM 'play <rock|paper|cisor>' to play rounds.");
    notifyOtherPlayerJoined(room, player);
}

void Bot::handleLeaveRoom(const std::string &player)
{
    std::map<std::string, std::string>::iterator playerRoomIterator = playerRoom.find(player);
    if (playerRoomIterator == playerRoom.end())
    {
        send_privmsg(player, "You're not in a room.");
        return;
    }

    std::string roomName = playerRoomIterator->second;
    Room &room = rooms[roomName];

    room.removePlayer(player);
    playerRoom.erase(playerRoomIterator);

    send_privmsg(player, "Left room '" + roomName + "'.");
    notifyRoomNotEnoughPlayers(room);

    if (room.isEmpty())
        rooms.erase(roomName);
}

void Bot::handleRoomStatus(const std::string &player)
{
    std::map<std::string, std::string>::iterator playerRoomIterator = playerRoom.find(player);
    if (playerRoomIterator == playerRoom.end())
    {
        send_privmsg(player, "You're not in a room.");
        return;
    }

    Room &room = rooms[playerRoomIterator->second];
    if (!room.isReady())
    {
        send_privmsg(player, "There are not enough players in the room.");
        return;
    }

    std::ostringstream statusMessage;
    statusMessage << "Room '" << room.getName() << "' players: [" 
                  << room.getPlayer1Nick() << ", " << room.getPlayer2Nick() << "] rounds: " 
                  << room.getRoundsPlayed() << "; score: " 
                  << room.getPlayer1Nick() << " " << room.getPlayer1Wins() << "-" 
                  << room.getPlayer2Wins() << " " << room.getPlayer2Nick()
                  << "; sets: " << room.getPlayer1Nick() << "=" << room.getPlayer1SetsWon() 
                  << ", " << room.getPlayer2Nick() << "=" << room.getPlayer2SetsWon();

    send_privmsg(player, statusMessage.str());
}

void Bot::leaveCurrentRoomIfAny(const std::string &player, const std::string &exceptRoom)
{
    std::map<std::string, std::string>::iterator currentRoomIterator = playerRoom.find(player);
    if (currentRoomIterator != playerRoom.end() && currentRoomIterator->second != exceptRoom)
    {
        std::string oldRoomName = currentRoomIterator->second;
        Room &oldRoom = rooms[oldRoomName];

        std::string otherPlayer;
        if (oldRoom.getPlayer1Nick() == player)
            otherPlayer = oldRoom.getPlayer2Nick();
        else if (oldRoom.getPlayer2Nick() == player)
            otherPlayer = oldRoom.getPlayer1Nick();

        oldRoom.removePlayer(player);
        playerRoom.erase(currentRoomIterator);
        send_privmsg(player, "You left room '" + oldRoomName + "'.");

        if (!otherPlayer.empty())
            send_privmsg(otherPlayer, player + " left your room '" + oldRoomName + "'. There are not enough players in the room.");

        if (oldRoom.isEmpty())
            rooms.erase(oldRoomName);
    }
}

void Bot::notifyOtherPlayerJoined(Room &room, const std::string &player)
{
    if (!room.getPlayer1Nick().empty() && room.getPlayer1Nick() != player)
        send_privmsg(room.getPlayer1Nick(), player + " joined your room '" + room.getName() + "'");

    if (!room.getPlayer2Nick().empty() && room.getPlayer2Nick() != player)
        send_privmsg(room.getPlayer2Nick(), player + " joined your room '" + room.getName() + "'");
}

void Bot::notifyRoomNotEnoughPlayers(Room &room)
{
    if (!room.getPlayer1Nick().empty())
        send_privmsg(room.getPlayer1Nick(), "There are not enough players in the room.");

    if (!room.getPlayer2Nick().empty())
        send_privmsg(room.getPlayer2Nick(), "There are not enough players in the room.");
}



void Bot::handleMultiplayerMove(const std::string &player, const std::string &move)
{
    std::map<std::string, std::string>::iterator playerRoomIterator = playerRoom.find(player);
    if (playerRoomIterator == playerRoom.end())
    {
        send_privmsg(player, "Join a room first: room create <name> or room join <name>");
        return;
    }
    Room &room = rooms[playerRoomIterator->second];
    if (!room.isReady())
    {
        send_privmsg(player, "There are not enough players in the room.");
        return;
    }
    if (!isValidMoveToken(move))
    {
        send_privmsg(player, "Invalid move. Use rock|paper|cisor.");
        return;
    }
    std::string normalizedMove = (move == "scissors") ? "cisor" : move;
    room.setPlayerChoice(player, normalizedMove);
    send_privmsg(player, "Move received. Waiting for the opponent...");
    if (room.bothChose())
    {
        notifyRoomBothChose(room);
    }
}

void Bot::notifyRoomBothChose(Room &room)
{
    if (!room.isReady())
    {
        return;
    }
    int outcome = compare_rps(room.getPlayer1Choice(), room.getPlayer2Choice());
    room.applyOutcome(outcome);
    std::string resultMessage;
    if (outcome > 0)
    resultMessage = room.getPlayer1Nick() + " wins this round";
    else if (outcome < 0)
    resultMessage = room.getPlayer2Nick() + " wins this round";
    else
        resultMessage = "Round is a tie";
    std::ostringstream roundMessage;
    roundMessage << "Choices: " << room.getPlayer1Nick() << "=" << room.getPlayer1Choice() << ", " << room.getPlayer2Nick() << "=" << room.getPlayer2Choice() << "; " << resultMessage << ". (" << room.getRoundsPlayed() << "/5)";
    if (!room.getPlayer1Nick().empty())
        send_privmsg(room.getPlayer1Nick(), roundMessage.str());
    if (!room.getPlayer2Nick().empty())
        send_privmsg(room.getPlayer2Nick(), roundMessage.str());
    room.clearChoices();

    if (room.isOver())
    {
        if (room.getPlayer1Wins() > room.getPlayer2Wins())
        {
            room.addPlayer1SetWin();
        }
        else if (room.getPlayer2Wins() > room.getPlayer1Wins())
        {
            room.addPlayer2SetWin();
        }
        std::ostringstream finalMessage;
        finalMessage << "Set finished. Round score: " << room.getPlayer1Nick() << " " << room.getPlayer1Wins() << " - " << room.getPlayer2Wins() << " " << room.getPlayer2Nick() << 
               ", ties=" << room.getRoundTies() << "; Sets: " << room.getPlayer1Nick() << "=" << room.getPlayer1SetsWon() << ", " << room.getPlayer2Nick() << "=" << room.getPlayer2SetsWon();
        if (!room.getPlayer1Nick().empty())
            send_privmsg(room.getPlayer1Nick(), finalMessage.str());
        if (!room.getPlayer2Nick().empty())
            send_privmsg(room.getPlayer2Nick(), finalMessage.str());
        room.resetRoundCounters();
    }
}

void Bot::send_room_scoreboard(const std::string &player)
{
    std::map<std::string, std::string>::iterator playerRoomIterator = playerRoom.find(player);
    if (playerRoomIterator == playerRoom.end())
    {
        send_privmsg(player, "You're not in a room. Use 'room status' or personal 'score'.");
        return;
    }
    Room &room = rooms[playerRoomIterator->second];

    if (!room.isReady())
    {
        send_privmsg(player, "There are not enough players in the room.");
        return;
    }

    std::string player1Name = room.getPlayer1Nick();
    int player1Wins = room.getPlayer1Wins();
    int player1Losses = room.getPlayer2Wins();
    int player1Ties = room.getRoundTies();
    int player1Games = room.getRoundsPlayed();
    int player1Sets = room.getPlayer1SetsWon();

    std::string player2Name = room.getPlayer2Nick();
    int player2Wins = room.getPlayer2Wins();
    int player2Losses = room.getPlayer1Wins();
    int player2Ties = room.getRoundTies();
    int player2Games = room.getRoundsPlayed();
    int player2Sets = room.getPlayer2SetsWon();

    bool isPlayer1First = true;
    if (player2Sets > player1Sets)
    {
        isPlayer1First = false;
    }
    else if (player2Sets == player1Sets && player2Wins > player1Wins)
    {
        isPlayer1First = false;
    }
    else if (player2Sets == player1Sets && player2Wins == player1Wins)
    {
        if (player2Ties > player1Ties)
            isPlayer1First = false;
        else if (player2Ties == player1Ties && player2Name < player1Name)
            isPlayer1First = false;
    }

    std::ostringstream scoreboard;
    scoreboard << "+----------------------------------------------+-----+-----+-----+--------+------+\r\n";
    scoreboard << "| Rank | Player               |  W  |  L  |  T  | Games  | Sets |\r\n";
    scoreboard << "+----------------------------------------------+-----+-----+-----+--------+------+\r\n";
    for (int rank = 1; rank <= 2; ++rank)
    {
        std::string name; int wins; int losses; int ties; int games; int sets;
        if ((rank == 1 && isPlayer1First) || (rank == 2 && !isPlayer1First))
        {
            name = player1Name; wins = player1Wins; losses = player1Losses; ties = player1Ties; games = player1Games; sets = player1Sets;
        }
        else
        {
            name = player2Name; wins = player2Wins; losses = player2Losses; ties = player2Ties; games = player2Games; sets = player2Sets;
        }
        std::ostringstream nameColumn;
        nameColumn << std::left << std::setw(20) << name;
        scoreboard << "|  " << rank << "   | " << nameColumn.str() << " | "
            << std::setw(3) << wins << " | "
            << std::setw(3) << losses << " | "
            << std::setw(3) << ties << " | "
            << std::setw(6) << games << " | "
            << std::setw(4) << sets << " |\r\n";
    }
    scoreboard << "+----------------------------------------------+-----+-----+-----+--------+------+\r\n";

    std::istringstream scoreboardStream(scoreboard.str());
    std::string line;
    while (std::getline(scoreboardStream, line))
    {
        if (!line.empty() && line[line.size()-1] == '\r')
            line.erase(line.size()-1);
        if (!room.getPlayer1Nick().empty())
            send_privmsg(room.getPlayer1Nick(), line);
        if (!room.getPlayer2Nick().empty() && room.getPlayer2Nick() != room.getPlayer1Nick())
            send_privmsg(room.getPlayer2Nick(), line);
    }
}
