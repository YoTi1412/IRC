#pragma once

// C / C++ standard headers
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

// Common project CRLF macro (use everywhere for line endings)
#define CRLF "\r\n"

// POSIX / networking headers
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

// Centralized project headers
// Including these here allows source files to just include "Includes.hpp"
// and get access to the common project headers.
#include "Channel.hpp"
#include "Client.hpp"
#include "Command.hpp"
#include "Logger.hpp"
#include "Message.hpp"
#include "Replies.hpp"
#include "Server.hpp"
#include "Utils.hpp"
