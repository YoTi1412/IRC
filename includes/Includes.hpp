#pragma once

#include <iostream>
#include <vector>
#include <map>
#include <list>
#include <set>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdlib>
#include <stdexcept>
#include <ctime>
#include <sstream>
#include <cerrno>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>

#include <signal.h>

// Forward declarations
class Server;
class Client;
class Channel;
class Message;
class Utils;
class Logger;
