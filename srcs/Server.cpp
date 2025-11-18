#include "Server.hpp"
#include "Client.hpp"
#include "Command.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include <cstring>

bool Server::signal = false;

Server::Server(const std::string &portStr, const std::string &password)
    : epfd(-1) {
  validateArgs(portStr, password);
  name = "ircserv";
  port = std::atoi(portStr.c_str());
  this->password = password;
  createdtime = Utils::formatTime(time(NULL));
  Logger::info("Server instance created with port " + portStr +
               " and password set.");
}

void Server::validateArgs(const std::string &portStr,
                          const std::string &password) {
  checkEmptyArgs(portStr, password);
  validatePort(portStr);
  validatePassword(password);
}

void Server::checkEmptyArgs(const std::string &portStr, const std::string &password) {
    if (portStr.empty() || password.empty()) {
        throw std::invalid_argument("Arguments are empty!");
    }
}

void Server::validatePort(const std::string &portStr) {
    if (!Utils::isValidPort(portStr.c_str())) {
        throw std::invalid_argument("Invalid port number. Must be between 1024 and 65535.");
    }
}

void Server::validatePassword(const std::string &password) {
    if (!Utils::isValidPassword(password)) {
        throw std::invalid_argument("Invalid password. No spaces or non-printable characters allowed.");
    }
}

Server::~Server() {
  cleanupAllClients();
  cleanupAllChannels();
  closeSocket();
  if (epfd >= 0) {
    close(epfd);
    Logger::info("Epoll file descriptor closed.");
  }
  logShutdown();
}

void Server::cleanupAllChannels() {
  for (std::map<std::string, Channel *>::iterator it = channels.begin();
       it != channels.end(); ++it) {
    delete it->second;
  }
  channels.clear();
  Logger::info("All channels cleaned up.");
}

std::map<std::string, Channel *> &Server::getChannels() { return channels; }

void Server::cleanupAllClients() {
  std::map<int, Client *>::iterator it = clients.begin();
  while (it != clients.end()) {
    close(it->first);
    delete it->second;
    ++it;
  }
  clients.clear();
  Logger::info("All clients cleaned up.");
}

void Server::closeSocket() {
  if (sock_fd > 0) {
    close(sock_fd);
    Logger::info("Server socket closed.");
  }
}

void Server::logShutdown() {
  Logger::info("Server on port " + Utils::intToString(port) +
               " is shutting down.");
}

const std::string &Server::getPassword() const { return password; }

const std::string &Server::getCreatedTime() const { return createdtime; }

std::map<int, Client *> &Server::getClients() { return this->clients; }

const std::string &Server::getName() const { return name; }

Client *Server::getClientByNickname(const std::string &nickname) const {
  for (std::map<int, Client *>::const_iterator it = clients.begin();
       it != clients.end(); ++it) {
    if (it->second->getNickname() == nickname) {
      return it->second;
    }
  }
  return NULL;
}

// ------------------- Socket Initialization -------------------

void Server::serverInit() {
  increaseFdLimit();
  createSocket();
  configureServerAddress();
  setReuseAddr();
  bindSocket();
  listenOnSocket();
  initEpoll();
  logInitialization();
}

void Server::increaseFdLimit() {
  struct rlimit rlim;
  if (getrlimit(RLIMIT_NOFILE, &rlim) == 0) {
    Logger::info("Current FD limit: soft=" + Utils::intToString(rlim.rlim_cur) +
                 ", hard=" + Utils::intToString(rlim.rlim_max));

    rlim_t newLimit;
    if (rlim.rlim_max == RLIM_INFINITY) {
      newLimit = 10000;
    } else if (rlim.rlim_max > 10000) {
      newLimit = 10000;
    } else {
      newLimit = rlim.rlim_max;
    }

    if (rlim.rlim_cur < newLimit) {
      rlim.rlim_cur = newLimit;
      if (setrlimit(RLIMIT_NOFILE, &rlim) == 0) {
        Logger::info("Successfully increased FD limit to: " +
                     Utils::intToString(newLimit));
      } else {
        Logger::warning("Failed to increase FD limit: " +
                        std::string(strerror(errno)));
      }
    }
  } else {
    Logger::warning("Failed to get current FD limit: " +
                    std::string(strerror(errno)));
  }
}

void Server::createSocket() {
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
    }
    Logger::info("Socket created successfully.");
}

void Server::configureServerAddress() {
    std::memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);
    Logger::debug("Server address configured for port " + Utils::intToString(port));
}

void Server::setReuseAddr() {
    int optval = 1;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        throw std::runtime_error("Failed to set SO_REUSEADDR: " + std::string(strerror(errno)));
    }
    Logger::debug("SO_REUSEADDR option enabled on socket.");
}

void Server::bindSocket() {
    if (bind(sock_fd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        throw std::runtime_error("Failed to bind socket: " + std::string(strerror(errno)));
    }
    Logger::info("Socket bound successfully.");
}

void Server::listenOnSocket() {
    if (listen(sock_fd, SOMAXCONN) < 0) {
        throw std::runtime_error("Failed to listen on socket: " + std::string(strerror(errno)));
    }
    Logger::info("Server listening on socket with maximum connections.");
}

void Server::initEpoll() {
    epfd = epoll_create1(EPOLL_CLOEXEC);
    if (epfd < 0) {
        throw std::runtime_error("Failed to create epoll instance: " + std::string(strerror(errno)));
    }

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = sock_fd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, sock_fd, &ev) < 0) {
        throw std::runtime_error("Failed to add socket to epoll: " + std::string(strerror(errno)));
    }
    Logger::debug("Epoll instance created and socket added successfully.");
}

void Server::logInitialization() {
  Logger::info("Server initialized on port " + Utils::intToString(port));
}

// ------------------- Event Loop -------------------

void Server::serverRun() {
  struct epoll_event events[MAX_EVENTS];
  while (!signal) {
    int nfds = 0;
    waitForEvents(events, nfds);
    if (nfds > 0) {
      processedFds.clear();
      processEvents(events, nfds);
    }
  }
  Logger::info("Server run loop terminated due to signal.");
}

void Server::waitForEvents(struct epoll_event events[], int& nfds) {
    nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
    if (nfds < 0 && errno != EINTR) {
        throw std::runtime_error("Epoll wait failed: " + std::string(strerror(errno)));
    }
    if (nfds >= 0) {
        Logger::debug("Epoll wait returned " + Utils::intToString(nfds) + " ready file descriptors.");
    }
}

void Server::processEvents(struct epoll_event events[], int nfds) {
  for (int i = 0; i < nfds; ++i) {
    int fd = events[i].data.fd;
    uint32_t eventFlags = events[i].events;

    if (fd == sock_fd && (eventFlags & EPOLLIN)) {
      acceptNewConnection();
    } else {
      handleClientEvent(fd, eventFlags);
    }
  }
  Logger::debug("Processed events for " + Utils::intToString(nfds) +
                " file descriptors.");
}

void Server::handleClientEvent(int fd, uint32_t events) {
  if (events & (EPOLLHUP | EPOLLERR)) {
    Logger::debug("Client fd " + Utils::intToString(fd) +
                  " disconnected (EPOLLHUP/EPOLLERR)");
    handleClientDisconnect(fd);
  } else if (events & EPOLLIN) {
    handleClientData(fd);
  }
}

// ------------------- Client Connection -------------------

void Server::acceptNewConnection() {
  sockaddr_in clientAddr;
  socklen_t clientLen = sizeof(clientAddr);
  int clientFd = accept(sock_fd, (struct sockaddr *)&clientAddr, &clientLen);
  handleAcceptResult(clientFd, clientAddr);
}

void Server::handleAcceptResult(int clientFd, sockaddr_in &clientAddr) {
  if (clientFd < 0) {
    if (errno == EMFILE || errno == ENFILE) {
      Logger::warning("Accept failed due to FD limit: " +
                      std::string(strerror(errno)));
    } else {
      Logger::warning("Accept failed: " + std::string(strerror(errno)));
    }
    return;
  }
  configureNewClient(clientFd, clientAddr);
}

void Server::configureNewClient(int clientFd, sockaddr_in &clientAddr) {
  Utils::setnonblocking(clientFd);
  Client *client = createNewClient(clientFd, clientAddr);
  clients[clientFd] = client;

  if (tryHandleHttpClient(clientFd)) {
    return;
  }

  // Check if client still exists after tryHandleHttpClient (could be deleted by
  // QUIT command)
  std::map<int, Client *>::iterator clientIt = clients.find(clientFd);
  if (clientIt == clients.end()) {
    return; // Client was disconnected during tryHandleHttpClient
  }

  sendIrcGreeting(clientIt->second);
  addClientToEpoll(clientFd);
}

static bool looksLikeHTTP(const char *buf) {
  return strncmp(buf, "GET ", 4) == 0 || strncmp(buf, "POST ", 5) == 0 ||
         strncmp(buf, "HEAD ", 5) == 0 || strncmp(buf, "PUT ", 4) == 0 ||
         strncmp(buf, "DELETE ", 7) == 0 || strncmp(buf, "OPTIONS ", 8) == 0 ||
         strncmp(buf, "TRACE ", 6) == 0 || strncmp(buf, "CONNECT ", 8) == 0;
}

bool Server::tryHandleHttpClient(int clientFd) {
  char buffer[BUFFER_SIZE] = {0};
  int bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);
  if (bytesRead <= 0) {
    if (bytesRead == 0 ||
        (bytesRead < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))) {
      return false; // No data, assume IRC
    }
    Logger::warning("Immediate read error on fd: " +
                    Utils::intToString(clientFd) + ", " + strerror(errno));
    handleClientDisconnect(clientFd);
    return true;
  }

  buffer[bytesRead] = '\0';
  if (looksLikeHTTP(buffer)) {
    sendHttpResponse(clientFd);
    handleClientDisconnect(clientFd);
    return true;
  }

  // Not HTTP, process as IRC
  std::map<int, Client *>::iterator clientIt = clients.find(clientFd);
  if (clientIt == clients.end()) {
    return true; // Client was disconnected, treat as handled
  }
  Client *client = clientIt->second;
  client->appendToCommandBuffer(buffer);
  processClientBuffer(clientFd);
  return false;
}

void Server::sendIrcGreeting(Client *client) {
  client->sendWelcomeHowTo(client->getFd());
  client->setGreeted(true);
}

void Server::addClientToEpoll(int clientFd) {
  // Check if client still exists before adding to epoll
  std::map<int, Client *>::iterator clientIt = clients.find(clientFd);
  if (clientIt == clients.end()) {
    return; // Client was disconnected, don't add to epoll
  }

  struct epoll_event ev;
  ev.events = EPOLLIN | EPOLLHUP | EPOLLERR;
  ev.data.fd = clientFd;
  if (epoll_ctl(epfd, EPOLL_CTL_ADD, clientFd, &ev) < 0) {
    Logger::warning("Failed to add client fd " + Utils::intToString(clientFd) +
                    " to epoll: " + strerror(errno));
    handleClientDisconnect(clientFd);
  } else {
    Logger::debug("Added client fd " + Utils::intToString(clientFd) +
                  " to epoll.");
  }
}

Client *Server::createNewClient(int clientFd, sockaddr_in &clientAddr) {
  Client *client = new Client();
  client->setFd(clientFd);
  char ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(clientAddr.sin_addr), ip, INET_ADDRSTRLEN);
  client->setIPAddress(ip);
  logNewConnection(clientFd, ip, ntohs(clientAddr.sin_port));
  return client;
}

void Server::logNewConnection(int clientFd, const char *ip, int port) {
  Logger::info("New client connected, fd: " + Utils::intToString(clientFd) +
               ", IP: " + ip + ", Port: " + Utils::intToString(port));
}

// ------------------- Client Data Handling -------------------

void Server::handleClientData(int fd) {
  char buffer[BUFFER_SIZE] = {0};
  int bytesRead = read(fd, buffer, sizeof(buffer) - 1);
  processReadResult(fd, buffer, bytesRead);
}

void Server::processReadResult(int fd, char *buffer, int bytesRead) {
  if (bytesRead < 0) {
    handleReadError(fd);
  } else if (bytesRead == 0) {
    handleClientDisconnect(fd);
  } else {
    handleReadSuccess(fd, buffer, bytesRead);
  }
}

void Server::handleReadError(int fd) {
  if (errno != EAGAIN && errno != EWOULDBLOCK) {
    Logger::warning("Read error on fd: " + Utils::intToString(fd) + ", " +
                    strerror(errno));
  }
  handleClientDisconnect(fd);
}

void Server::handleClientDisconnect(int fd) {
  if (processedFds.find(fd) != processedFds.end()) {
    return; // Already processed
  }
  processedFds.insert(fd);

  // Remove from epoll
  if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL) < 0) {
    Logger::debug("Failed to remove fd " + Utils::intToString(fd) +
                  " from epoll (may already be removed)");
  }

  std::map<int, Client *>::iterator clientIt = clients.find(fd);
  if (clientIt != clients.end()) {
    Client *client = clientIt->second;
    // Remove from channels
    std::map<std::string, Channel *>::iterator chanIt = channels.begin();
    while (chanIt != channels.end()) {
      Channel *channel = chanIt->second;
      channel->removeMember(client);
      if (channel->getMemberCount() == 0) {
        delete channel;
        channels.erase(chanIt++);
      } else {
        ++chanIt;
      }
    }
    delete client;
    clients.erase(clientIt);
  }

  if (fd > 0) {
    close(fd);
  }
  Logger::info("Client disconnected, fd: " + Utils::intToString(fd));
}

void Server::sendHttpResponse(int fd) {
  const char *http = "HTTP/1.1 200 OK\r\n"
                     "Content-Type: text/plain\r\n"
                     "Content-Length: 31\r\n"
                     "Connection: close\r\n"
                     "\r\n"
                     "This is an IRC server mate ;)\r\n";
  send(fd, http, strlen(http), MSG_NOSIGNAL);
}

void Server::handleReadSuccess(int fd, char *buffer, int bytesRead) {
  buffer[bytesRead] = '\0';
  Logger::debug("Read " + Utils::intToString(bytesRead) + " bytes from fd " +
                Utils::intToString(fd) + ": " + buffer);

  if (looksLikeHTTP(buffer)) {
    sendHttpResponse(fd);
    handleClientDisconnect(fd);
    return;
  }

  appendToClientBuffer(fd, buffer);
  processClientBuffer(fd);
}

void Server::appendToClientBuffer(int fd, const char *data) {
  if (clients.find(fd) != clients.end()) {
    clients[fd]->getCommandBuffer() += data;
  }
}

void Server::processClientBuffer(int fd) {
  Client *client = clients[fd];
  std::string &buf = client->getCommandBuffer();

  while (hasNextMessage(buf)) {
    std::string msg = extractNextMessage(buf);
    std::list<std::string> cmd = parseMessage(msg);
    if (!cmd.empty()) {
      executeCommand(fd, cmd);
    }
  }
}

bool Server::hasNextMessage(const std::string &buffer) {
  return buffer.find('\n') != std::string::npos;
}

std::string Server::extractNextMessage(std::string &buffer) {
  size_t pos = buffer.find('\n');
  std::string message = extractMessage(buffer, pos);
  buffer.erase(0, pos + 1);
  return message;
}

std::string Server::extractMessage(const std::string &buffer, size_t pos) {
  size_t end = (pos > 0 && buffer[pos - 1] == '\r') ? pos - 1 : pos;
  return buffer.substr(0, end);
}

std::list<std::string> Server::parseMessage(const std::string &message) {
  std::list<std::string> cmdList;
  size_t colonPos = message.find(" :");
  std::string prefix = message;
  std::string trailing;
  if (colonPos != std::string::npos) {
    prefix = message.substr(0, colonPos);
    trailing = message.substr(colonPos + 2);
  }

  tokenizePrefix(prefix, cmdList);
  if (colonPos != std::string::npos) {
    cmdList.push_back(trailing);
  }
  return cmdList;
}

void Server::tokenizePrefix(const std::string &prefix,
                            std::list<std::string> &cmdList) {
  std::istringstream iss(prefix);
  std::string token;
  while (iss >> token) {
    cmdList.push_back(token);
  }
}

// ------------------- Command Execution -------------------

void Server::executeCommand(int fd, std::list<std::string> cmdList) {
  if (cmdList.empty()) {
    return;
  }

  std::string cmd = cmdList.front();
  if (!isUpperCase(cmd)) {
    sendInvalidCommandError(fd, cmd);
    return;
  }

  std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
  Client *client = clients[fd];
  dispatchCommand(cmd, cmdList, client);
}

void Server::sendInvalidCommandError(int fd, const std::string &cmd) {
  clients[fd]->sendReply(":ircserv " ERR_UNKNOWNCOMMAND " * " + cmd +
                         " :Commands must be uppercase\r\n");
  Logger::warning("Invalid command received from fd " + Utils::intToString(fd) +
                  ": " + cmd);
}

void Server::dispatchCommand(const std::string &cmd,
                             std::list<std::string> cmdList, Client *client) {
  if (cmd == "PASS") {
    handlePass(cmdList, client, this);
  } else if (cmd == "NICK") {
    handleNick(cmdList, client, this);
  } else if (cmd == "USER") {
    handleUser(cmdList, client, this);
  } else if (cmd == "JOIN") {
    handleJoin(cmdList, client, this);
  } else if (cmd == "PRIVMSG") {
    handlePrivmsg(cmdList, client, this);
  } else if (cmd == "PART") {
    handlePart(cmdList, client, this);
  } else if (cmd == "MODE") {
    handleMode(cmdList, client, this);
  } else if (cmd == "INVITE") {
    handleInvite(cmdList, client, this);
  } else if (cmd == "NAMES") {
    handleNames(cmdList, client, this);
  } else if (cmd == "TOPIC") {
    handleTopic(cmdList, client, this);
  } else if (cmd == "KICK") {
    handleKick(cmdList, client, this);
  } else if (cmd == "QUIT") {
    handleQuit(cmdList, client, this);
  } else if (cmd == "PING") {
    handlePing(cmdList, client, this);
  } else {
    sendUnknownCommandError(client, cmd);
  }
}

void Server::sendUnknownCommandError(Client *client, const std::string &cmd) {
  std::string nick =
      client->getNickname().empty() ? "*" : client->getNickname();
  client->sendReply(":ircserv " ERR_UNKNOWNCOMMAND " " + nick + " " + cmd +
                    " :Unknown command\r\n");
  Logger::warning("Unknown command received from client " +
                  client->getNickname() + ": " + cmd);
}

bool Server::isUpperCase(const std::string &str) {
  for (size_t i = 0; i < str.size(); ++i) {
    if (!std::isupper(str[i])) {
      return false;
    }
  }
  return true;
}

void Server::removeChannel(const std::string &channelName) {
  std::map<std::string, Channel *>::iterator it = channels.find(channelName);
  if (it != channels.end()) {
    std::string name_copy = channelName;
    delete it->second;
    channels.erase(it);
    Logger::info("Channel " + name_copy + " deleted and erased.");
  }
}

// ------------------- Signal Handling -------------------

void Server::sigHandler(int sig) {
  (void)sig;
  Logger::warning("Signal received! Stopping server...");
  signal = true;
}
