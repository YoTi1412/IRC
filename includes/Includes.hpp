#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <list>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdlib>
#include <stdexcept>
#include <ctime>
#include <sstream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>

#include <signal.h>

#include "Server.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "Message.hpp"
#include "Replies.hpp"

class Server;
class Client;
class Channel;
class Message;
class Utils;
class Logger;
