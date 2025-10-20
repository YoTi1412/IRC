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


void Bot::handleHelpCommand(const std::string &sender)
{
    send_privmsg(sender, "Welcome to the Cisor Bot!");
    send_privmsg(sender, "This bot plays Rock-Paper-Scissors with you.");
    send_privmsg(sender, "To play, send PRIVMSG to me with: rock, paper, or cisor.");
    send_privmsg(sender, "If you want to play against your friend, we have multiplayer rooms too!");
    send_privmsg(sender, "Usage: room <create|join|leave|status>");
    send_privmsg(sender, "Rules: Room must have 2 players to start, and you must type play before your move.");
    send_privmsg(sender, "Example: PRIVMSG cisor :play rock");
}


void Bot::handleLine(const std::string &line)
{
    std::string sender, target, message;
    if (!extractPrivmsg(line, sender, target, message))
    {
        return;
    }

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
        if (cmd[0] == "help")
        {
            handleHelpCommand(sender);
            return ;
        }
    }

    if (isScore(message))
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

bool Bot::extractPrivmsg(const std::string &line, std::string &senderNick, std::string &targetChannel, std::string &msgContent) const
{
    size_t privmsgPos = line.find(" PRIVMSG ");
    if (privmsgPos == std::string::npos)
        return false;

    size_t messageStart = line.find(" :", privmsgPos);
    if (messageStart == std::string::npos)
        return false;

    targetChannel = line.substr(privmsgPos + 9, messageStart - (privmsgPos + 9));

    msgContent = line.substr(messageStart + 2);

    senderNick.clear();
    if (!line.empty() && line[0] == ':')
    {
        size_t exclMarkPos = line.find('!');
        if (exclMarkPos != std::string::npos && exclMarkPos > 1)
        {
            senderNick = line.substr(1, exclMarkPos - 1);
        }
        else
        {
            size_t firstSpace = line.find(' ');
            if (firstSpace != std::string::npos && firstSpace > 1)
                senderNick = line.substr(1, firstSpace - 1);
        }
    }

    return !senderNick.empty();
}


bool Bot::parseCommand(const std::string &msg, std::vector<std::string> &out) const
{
    out.clear();
    std::istringstream iss(msg);
    std::string tok;
    while (iss >> tok)
    {
        out.push_back(tok);
    }
    return !out.empty();
}

void Bot::auth(const std::string &password)
{
    send_all(std::string("PASS ") + password + "\r\n");
    send_all(std::string("NICK cisor\r\n"));
    send_all(std::string("USER bot 0 * :cisor bot\r\n"));
}

void Bot::mainLoop()
{
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
}

int Bot::initSocket(struct sockaddr_in &srv)
{
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
    return 0;
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

    if (initSocket(srv))
        return 1;
    std::signal(SIGPIPE, SIG_IGN);
    auth(password);
    mainLoop();
    
    return 0;
}
