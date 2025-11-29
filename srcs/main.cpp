#include "Includes.hpp"

int main(int argc, char** argv)
{
    try {
        if (argc != 3) {
            throw std::invalid_argument("Usage: ./ircserv <port> <password>");
        }
        
        Server server(argv[1], argv[2]);
        Utils::setupSignalHandler();
        server.serverInit();
        
        Utils::displayBanner();
        
        server.serverRun();
    } catch (const std::exception& e) {
        Logger::error(e);
        return 1;
    }
    return 0;
}
