#include "../includes/Bot.hpp"
#include <iostream>
#include <cstdlib>

int main(int argc, char **argv) {
    if (argc < 4)
    {
        std::cerr << "Usage: " << argv[0] << " <host> <port> <password>\n";
        return 1;
    }
    std::string host = argv[1];
    int port = atoi(argv[2]);
    std::string password = argv[3];
    
    Bot bot(host, port, password);
    return bot.run();
}
