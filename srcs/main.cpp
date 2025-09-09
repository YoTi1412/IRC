#include "Includes.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include "Server.hpp"

int main(int argc, char** argv)
{
    if (argc != 3) {
        Logger::error("Usage: ./ircserv <port> <password>");
        return 1;
    }
    try {
        Server server(argv[1], argv[2]);
        Utils::setupSignalHandler();
        server.serverInit();
        server.serverRun();
    } catch (const std::exception& e) {
        Logger::error(e);
        return 1;
    }
    return 0;
}
