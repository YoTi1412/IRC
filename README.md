# ft_irc Documentation

## Project Overview
This project implements an IRC (Internet Relay Chat) server as per the 42 school curriculum. The goal is to create a server (`ircserv`) that handles multiple client connections using non-blocking I/O, adheres to key IRC protocol elements (RFC 2812), and supports features like authentication, channels, private messages, and channel operator commands.

- **Program Name**: ircserv
- **Language/Standard**: C++98
- **Key Constraints**: No forking, non-blocking I/O with `poll()` (or equivalent), TCP/IP communication.
- **Arguments**: `./ircserv <port> <password>`
- **Testing**: Use an existing IRC client (e.g., irssi, HexChat) to connect and test. No custom client is required.
- **Implemented Features** (to be updated):
  - Server socket setup, binding, listening.
  - Client connection handling with `poll()`.
  - Basic logging and utils.
  - (Placeholder: Add commands like PASS, NICK, USER as implemented.)

Reference: IRC protocol (RFC 2812). For full project requirements, see `irc.pdf`.

## Code Structure
- **main.cpp**: Entry point, initializes server with port/password, sets up signal handlers, runs the server loop.
- **Server.hpp/cpp**: Core server class handling socket creation, polling, client acceptance, data handling, and shutdown.
- **Client.hpp/cpp**: Manages individual clients (FD, IP, nickname, username, registration status). Includes placeholder for command processing.
- **Utils.hpp/cpp**: Helper functions (port/password validation, time formatting, signal setup, int-to-string, non-blocking FD).
- **Logger.hpp/cpp**: Static logging for debug, info, warning, error messages with color codes.
- **Includes.hpp**: Common includes and forward declarations.

## Setup and Usage
### Prerequisites
- Compiler: clang++ or g++ with C++98 support.
- OS: Tested on [your OS, e.g., macOS/Linux]. Note: On macOS, use `fcntl(fd, F_SETFL, O_NONBLOCK)` for non-blocking.

### Building
Run `make` (assuming you add a Makefile as required by the subject).

### Running
```
./ircserv <port> <password></password></port>
```
- `<port>`: Valid port between 1024-65535 (e.g., 6667).
- `<password>`: Server connection password (no spaces/non-printables).

### Example:
```
./ircserv 6667 mypass
```

## Architecture Notes
- **Event Loop**: Uses `poll()` for handling server socket and client FDs (POLLIN).
- **Client Management**: std::map<int, Client*> for clients by FD.
- **Command Handling**: Incoming data buffered in `handleClientData()`. Processed in `Client::processCommand()` (TODO: Implement parsing for \r\n delimited commands).
- **Future**: Add Channel class for managing channels, modes, users.

## Implemented Commands
(Placeholder – update as you go)
- None yet. Next: Registration (PASS, NICK, USER).

## TODOs and Next Steps
- Implement command parser in Client.cpp.
- Add Channel class.
- Handle IRC responses/errors.
- Implement required commands: JOIN, PRIVMSG, KICK, INVITE, TOPIC, MODE (i/t/k/o/l).

## Resources
- IRC RFC: https://datatracker.ietf.org/doc/html/rfc2812
- Testing Client: irssi[](https://irssi.org/)
