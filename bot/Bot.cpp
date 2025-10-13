#include "Bot.hpp"
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <csignal>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <vector>

#define BUFFER 1024

Bot::Bot(const std::string &host, int port, const std::string &password) : host(host), port(port), password(password), sock(-1)
{
    std::srand(::time(NULL));
}

Bot::~Bot()
{
    if (sock >= 0) close(sock);
}

void Bot::send_all(const std::string &msg)
{
    if (sock >= 0)
    {
        send(sock, msg.c_str(), msg.length(), 0);
    }
}

void Bot::send_privmsg(const std::string &target, const std::string &msg)
{
    send_all(std::string("PRIVMSG ") + target + " :" + msg + "\r\n");
}

std::string Bot::choose_rps()
{
    int v = std::rand() % 3;
    if (v == 0)
        return "rock";
    if (v == 1)
        return "paper";
    return "cisor";
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

static void trim_crlf(std::string &s)
{
    while (!s.empty())
    {
        char c = s[s.size()-1];
        if (c == '\r' || c == '\n')
            s.erase(s.size()-1);
        else
            break;
    }
}

static std::string toLowerStr(const std::string &s)
{
    std::string out = s;
    for (size_t i = 0; i < out.size(); ++i)
        out[i] = (char)std::tolower((unsigned char)out[i]);
    return out;
}

static std::string trimTrailingPunct(const std::string &s)
{
    if (s.empty()) return s;
    char last = s[s.size() - 1];
    if (last == ':' || last == ',')
        return s.substr(0, s.size() - 1);
    return s;
}

static bool isValidMoveToken(const std::string &t)
{
    return t == "rock" || t == "paper" || t == "cisor" || t == "scissors";
}

void Bot::handleLine(const std::string &line)
{
    std::string token;
    if (isPing(line, token))
    {
        send_all(std::string("PONG ") + token + "\r\n");
        return;
    }

    std::string sender, target, message;
    if (!extractPrivmsg(line, sender, target, message))
    {
        return;
    }

    std::string lower = message;
    for (size_t i = 0; i < lower.size(); ++i)
        lower[i] = (char)std::tolower((unsigned char)lower[i]);

    std::vector<std::string> cmd;
    if (parseCommand(message, cmd) && !cmd.empty())
    {
        if (cmd[0] == "room")
        {
            handleMultiplayerCommand(sender, cmd);
            return;
        }
        if (cmd[0] == "scoreboard")
        {
            send_room_scoreboard(sender);
            return;
        }
        if (cmd[0] == "play" && cmd.size() >= 2)
        {
            handleMultiplayerMove(sender, cmd[1]);
            return;
        }
    }

    if (isScoreOrStats(lower))
    {
        handleScoreRequest(sender);
        return;
    }

    std::string playerTok;
    if (!parsePlayerMove(message, playerTok))
    {
        send_privmsg(sender, "Invalid move. How to play: send 'rock', 'paper' or 'cisor' (or 'scissors'). Example: PRIVMSG cisor :rock");
        return;
    }

    handleMoveRound(sender, playerTok);
}

bool Bot::isPing(const std::string &line, std::string &token) const
{
    if (line.find("PING ") == 0)
    {
        token = line.substr(5);
        return true;
    }
    return false;
}

bool Bot::extractPrivmsg(const std::string &line, std::string &sender, std::string &target, std::string &message) const {
    size_t pos = line.find(" PRIVMSG ");
    if (pos == std::string::npos)
        return false;
    size_t colon = line.find(" :", pos);
    if (colon == std::string::npos)
        return false;
    target = line.substr(pos + 9, colon - (pos + 9));
    message = line.substr(colon + 2);

    sender.clear();
    if (!line.empty() && line[0] == ':')
    {
        size_t excl = line.find('!');
        if (excl != std::string::npos && excl > 1)
            sender = line.substr(1, excl - 1);
        else
        {
            size_t sp = line.find(' ');
            if (sp != std::string::npos && sp > 1)
                sender = line.substr(1, sp - 1);
        }
    }
    return !sender.empty();
}

bool Bot::isScoreOrStats(const std::string &lower) const
{
    if (lower.find("score") != std::string::npos)
        return true;
    if (lower.find("stats") != std::string::npos)
        return true;
    return false;
}


bool Bot::parseCommand(const std::string &msg, std::vector<std::string> &out) const
{
    out.clear();
    std::istringstream iss(msg);
    std::string tok;
    while (iss >> tok)
    {
        tok = toLowerStr(trimTrailingPunct(tok));
        out.push_back(tok);
    }
    return !out.empty();
}

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
        // If user is already in a room, auto-leave it first
        std::map<std::string, std::string>::iterator cur = playerRoom.find(sender);
        if (cur != playerRoom.end())
        {
            std::string oldRn = cur->second;
            Room &old = rooms[oldRn];
            // Notify the other player (if any)
            std::string other;
            if (old.getP1() == sender)
                other = old.getP2();
            else if (old.getP2() == sender)
                other = old.getP1();
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
            if (old.getP1() == sender)
                other = old.getP2();
            else if (old.getP2() == sender)
                other = old.getP1();
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
        if (!r.getP1().empty() && r.getP1() != sender)
        {
            send_privmsg(r.getP1(), sender + " joined your room '" + rn + "'");
        }
        if (!r.getP2().empty() && r.getP2() != sender)
        {
            send_privmsg(r.getP2(), sender + " joined your room '" + rn + "'");
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
        if (!r.getP1().empty())
        {
            send_privmsg(r.getP1(), "There are not enough players in the room.");
        }
        if (!r.getP2().empty())
        {
            send_privmsg(r.getP2(), "There are not enough players in the room.");
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
        ss << "Room '" << r.getName() << "' players: [" << r.getP1() << ", " << r.getP2() << "] rounds: " << r.getPlayed()
           << "; score: " << r.getP1() << " " << r.getW1() << "-" << r.getW2() << " " << r.getP2()
           << "; sets: " << r.getP1() << "=" << r.getSetW1() << ", " << r.getP2() << "=" << r.getSetW2();
        send_privmsg(sender, ss.str());
    }
    else
    {
        send_privmsg(sender, "Unknown subcommand. Use: room <create|join|leave|status> [name]");
    }
}

void Bot::handleMultiplayerMove(const std::string &sender, const std::string &move)
{
    std::map<std::string, std::string>::iterator pr = playerRoom.find(sender);
    if (pr == playerRoom.end())
    {
        send_privmsg(sender, "Join a room first: room create <name> or room join <name>");
        return;
    }
    Room &r = rooms[pr->second];
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
    r.setChoice(sender, norm);
    send_privmsg(sender, "Move received. Waiting for the opponent...");
    if (r.bothChose())
    {
        notifyRoomBothChose(r);
    }
}

void Bot::notifyRoomBothChose(Room &r)
{
    if (!r.isReady())
    {
        // A player left or switched rooms; do not proceed.
        return;
    }
    int outcome = compare_rps(r.getC1(), r.getC2());
    r.applyOutcome(outcome);
    std::string msg;
    if (outcome > 0)
        msg = r.getP1() + " wins this round";
    else if (outcome < 0)
        msg = r.getP2() + " wins this round";
    else
        msg = "Round is a tie";
    std::ostringstream round;
    round << "Choices: " << r.getP1() << "=" << r.getC1() << ", " << r.getP2() << "=" << r.getC2() << "; " << msg << ". (" << r.getPlayed() << "/5)";
    if (!r.getP1().empty())
        send_privmsg(r.getP1(), round.str());
    if (!r.getP2().empty())
        send_privmsg(r.getP2(), round.str());
    r.clearChoices();

    if (r.isOver())
    {
        // Determine set winner, increment set counters, announce, then reset rounds (keep room open)
        if (r.getW1() > r.getW2())
        {
            r.addSetW1();
        }
        else if (r.getW2() > r.getW1())
        {
            r.addSetW2();
        }
        std::ostringstream fin;
        fin << "Set finished. Round score: " << r.getP1() << " " << r.getW1() << " - " << r.getW2() << " " << r.getP2() << 
               ", ties=" << r.getTies() << "; Sets: " << r.getP1() << "=" << r.getSetW1() << ", " << r.getP2() << "=" << r.getSetW2();
        if (!r.getP1().empty())
            send_privmsg(r.getP1(), fin.str());
        if (!r.getP2().empty())
            send_privmsg(r.getP2(), fin.str());
        // Reset per-set counters for next set
        r.resetRoundCounters();
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
    std::string p1n = r.getP1();
    int p1w = r.getW1();
    int p1l = r.getW2();
    int p1t = r.getTies();
    int p1g = r.getPlayed();
    int p1s = r.getSetW1();

    std::string p2n = r.getP2();
    int p2w = r.getW2();
    int p2l = r.getW1();
    int p2t = r.getTies();
    int p2g = r.getPlayed();
    int p2s = r.getSetW2();

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
        if (!r.getP1().empty())
            send_privmsg(r.getP1(), ln);
        if (!r.getP2().empty() && r.getP2() != r.getP1())
            send_privmsg(r.getP2(), ln);
    }
}
bool Bot::parsePlayerMove(const std::string &message, std::string &playerTok) const {
    std::istringstream iss(message);
    std::vector<std::string> tokens;
    std::string tok;
    while (iss >> tok) {
        tok = toLowerStr(trimTrailingPunct(tok));
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

int Bot::run()
{
    struct hostent *hp = gethostbyname(host.c_str());
    if (!hp)
    {
        perror("gethostbyname");
        return 1;
    }

    struct sockaddr_in srv;
    std::memset(&srv, 0, sizeof(srv));
    srv.sin_family = AF_INET;
    srv.sin_port = htons(port);
    std::memcpy(&srv.sin_addr, hp->h_addr, hp->h_length);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket");
        return 1;
    }

    if (connect(sock, (struct sockaddr*)&srv, sizeof(srv)) < 0)
    {
        perror("connect");
        close(sock);
        sock = -1;
        return 1;
    }

    std::signal(SIGPIPE, SIG_IGN);

    send_all(std::string("PASS ") + password + "\r\n");
    send_all(std::string("NICK cisor\r\n"));
    send_all(std::string("USER bot 0 * :cisor bot\r\n"));
    send_all(std::string("JOIN #general\r\n"));

    char buf[BUFFER+1];
    while (true) 
    {
        ssize_t n = recv(sock, buf, BUFFER, 0);
        if (n <= 0)
            break;
        buf[n] = '\0';
        std::string chunk(buf);
        std::istringstream iss(chunk);
        std::string line;
        while (std::getline(iss, line))
        {
            trim_crlf(line);
            if (line.empty())
                continue;
            handleLine(line);
        }
    }
    return 0;
}
