This document explains the behavior, responsibilities, interactions, and important implementation details of the server functions starting at `acceptNewConnection()` in `Server.cpp`. It is intended for maintainers who need to understand the connection lifecycle, how incoming data is processed, and how IRC commands are parsed and dispatched.

Table of contents
- Overview / high-level flow
- Connection acceptance and setup
  - `acceptNewConnection`
  - `handleAcceptResult`
  - `configureNewClient`
  - `createNewClient`
  - `logNewConnection`
  - `addClientToEpoll`
  - `sendIrcGreeting`
- HTTP detection and handling
  - `looksLikeHTTP`
  - `tryHandleHttpClient`
  - `sendHttpResponse`
- Reading and processing client data
  - `handleClientData`
  - `processReadResult`
  - `handleReadError`
  - `handleReadSuccess`
- Connection teardown
  - `handleClientDisconnect`
- Buffering and message parsing
  - `appendToClientBuffer`
  - `processClientBuffer`
  - `hasNextMessage`
  - `extractNextMessage`
  - `extractMessage`
  - `parseMessage`
  - `tokenizePrefix`
- Command execution and dispatch
  - `executeCommand`
  - `sendInvalidCommandError`
  - `dispatchCommand`
  - `sendUnknownCommandError`
  - `isUpperCase`
- Channel cleanup helper
  - `removeChannel`
- Signal handling
  - `sigHandler`
- Important implementation notes and best-practice hints
- How to extend / test

---

Overview / high-level flow
- The server listens on a socket and uses `epoll` for readiness notifications.
- When the listening socket becomes readable, the server calls `acceptNewConnection()` to accept a new client.
- After acceptance, the new socket is made non-blocking and a `Client` object is created and stored.
- The server tries to detect HTTP requests immediately. If the peer is an HTTP client, it responds and closes the connection; otherwise the connection is treated as an IRC client and added to `epoll`.
- Incoming data from IRC clients is read, appended to a per-client command buffer, and processed message-by-message.
- Each message is parsed into tokens (with special handling for the trailing parameter starting with `:`) and dispatched to the relevant IRC command handler.
- On socket errors, disconnects, or explicit `QUIT`, the server removes the client from epoll, channels, and deletes resources.

---

Connection acceptance and setup

- `acceptNewConnection()`
  - Purpose: Accepts one incoming connection from the listening socket.
  - Behavior: Calls `accept(sock_fd, ...)` and then delegates to `handleAcceptResult(clientFd, clientAddr)`.
  - Side effects: May log accept failures indirectly via `handleAcceptResult`.

- `handleAcceptResult(int clientFd, sockaddr_in &clientAddr)`
  - Purpose: Validate accept result and continue setup for valid fds.
  - Behavior:
    - If `accept` returned a negative fd: logs a warning; special-cases `EMFILE`/`ENFILE` (file descriptor exhaustion).
    - If `accept` succeeded: calls `configureNewClient`.
  - Side effects: No resource allocation on error; on success it continues client setup.

- `configureNewClient(int clientFd, sockaddr_in &clientAddr)`
  - Purpose: Complete configuration for a newly accepted client.
  - Behavior:
    - Makes the socket non-blocking (`Utils::setnonblocking`).
    - Creates a `Client` object via `createNewClient` and stores it in `clients[clientFd]`.
    - Calls `tryHandleHttpClient(clientFd)` to attempt an immediate HTTP probe (reads once with `MSG_DONTWAIT`).
      - If `tryHandleHttpClient` handles the connection (HTTP reply or disconnect), `configureNewClient` returns early.
      - If the client was removed during that handling, it detects absence in `clients` and returns.
    - If still present and not HTTP, calls `sendIrcGreeting` and `addClientToEpoll`.
  - Side effects: Allocates `Client` object; may send data (HTTP reply or greeting); adds to epoll.

- `createNewClient(int clientFd, sockaddr_in &clientAddr)`
  - Purpose: Instantiate and initialize a `Client`.
  - Behavior:
    - Allocates a `Client`.
    - Sets file descriptor, captures peer IP (`inet_ntop`), stores port information, logs the connection.
  - Side effects: Allocates memory; calls `logNewConnection`.

- `logNewConnection(int clientFd, const char *ip, int port)`
  - Purpose: Write an informational log entry about the new connection.
  - Behavior: Uses the `Logger` to record fd, IP and port.

- `addClientToEpoll(int clientFd)`
  - Purpose: Register the newly-created client's fd with `epoll` for future events.
  - Behavior:
    - Builds an `epoll_event` with `EPOLLIN | EPOLLHUP | EPOLLERR`.
    - Calls `epoll_ctl(EPOLL_CTL_ADD, clientFd, &ev)`.
    - On failure, logs and invokes `handleClientDisconnect`.
  - Side effects: Fd is tracked by `epoll` (or disconnected if registration fails).

- `sendIrcGreeting(Client *client)`
  - Purpose: Send welcome/how-to messages for IRC clients and set a `greeted` flag.
  - Behavior: Calls client-level send function(s) and marks the client greeted.

---

HTTP detection and handling

- `looksLikeHTTP(const char *buf)` (static)
  - Purpose: Quick heuristic to decide if a buffer contains an HTTP request line.
  - Behavior: Checks for prefixes `GET `, `POST `, or `HEAD `.
  - Note: This is intentionally simple — enough to distinguish browser/HTTP probes from IRC.

- `tryHandleHttpClient(int clientFd)`
  - Purpose: Probe the newly-accepted socket for immediate data that might be HTTP.
  - Behavior:
    - Calls `recv` with `MSG_DONTWAIT` into a temporary buffer.
    - If no data available (0 or EAGAIN/EWOULDBLOCK), return false (treat as IRC).
    - On read error (other than EAGAIN/EWOULDBLOCK): log and disconnect; return true (handled).
    - If data looks like HTTP (via `looksLikeHTTP`): call `sendHttpResponse` and disconnect; return true.
    - Otherwise: append read data to the client's command buffer and call `processClientBuffer`; return false (not HTTP).
  - Side effects: May send HTTP reply, remove client, or append data and process it as IRC.

- `sendHttpResponse(int fd)`
  - Purpose: Respond to HTTP probes with a short 200 OK text response and then close.
  - Behavior:
    - Sends a static HTTP response using `send(..., MSG_NOSIGNAL)` to avoid SIGPIPE.
    - Does not rely on keep-alive; connection is intended to be closed afterwards.

---

Reading and processing client data

- `handleClientData(int fd)`
  - Purpose: Entry point when `epoll` indicates data available on a client fd.
  - Behavior:
    - Calls `read(fd, ...)` into a buffer and forwards result to `processReadResult`.
  - Notes: Uses a fixed `BUFFER_SIZE` buffer; the code expects reads may return partial messages.

- `processReadResult(int fd, char *buffer, int bytesRead)`
  - Purpose: Decide how to proceed based on the result of a read.
  - Behavior:
    - If `bytesRead < 0`: call `handleReadError`.
    - If `bytesRead == 0`: client closed connection -> `handleClientDisconnect`.
    - If `bytesRead > 0`: `handleReadSuccess`.
  - Side effects: May disconnect or process data.

- `handleReadError(int fd)`
  - Purpose: Handle non-success `read` results.
  - Behavior:
    - If the error is not `EAGAIN`/`EWOULDBLOCK`, logs a warning.
    - Proceeds to `handleClientDisconnect` to clean up.
  - Note: The server opts to disconnect on read errors.

- `handleReadSuccess(int fd, char *buffer, int bytesRead)`
  - Purpose: Process positive reads.
  - Behavior:
    - Null-terminates the buffer for logging.
    - If data looks like HTTP (via `looksLikeHTTP`): send HTTP response and disconnect.
    - Otherwise: calls `appendToClientBuffer(fd, buffer)` followed by `processClientBuffer(fd)`.

---

Connection teardown

- `handleClientDisconnect(int fd)`
  - Purpose: Cleanly remove a client and free associated resources.
  - Behavior:
    - Uses a `processedFds` set to avoid double-processing the same fd in a single epoll loop iteration.
    - Removes the fd from `epoll` (ignores failure if already removed).
    - Finds the `Client` in `clients` map:
      - For every `Channel` in `channels`, calls `channel->removeMember(client)`. If a channel becomes empty, deletes it and erases it from the `channels` map.
      - Deletes the `Client` object and erases it from the `clients` map.
    - Closes the file descriptor.
    - Logs the disconnect.
  - Important: This removes all channel memberships and deallocates memory. Because channel erasure happens while iterating, it uses a pattern that safely erases while iterating.

---

Buffering and message parsing

- `appendToClientBuffer(int fd, const char *data)`
  - Purpose: Add newly read raw bytes to the per-client command buffer.
  - Behavior: Finds the `Client` object and appends `data` to `client->getCommandBuffer()`.

- `processClientBuffer(int fd)`
  - Purpose: Extract complete IRC messages and process them.
  - Behavior:
    - Repeatedly checks `hasNextMessage(buffer)` and while true:
      - `extractNextMessage` to obtain the next line/message.
      - `parseMessage` to split into tokens, preserving the IRC trailing parameter semantics.
      - If tokens are non-empty, call `executeCommand(fd, cmd)`.
  - Note: Message boundaries are newline-based; CRLF (`\r\n`) is handled by `extractMessage`.

- `hasNextMessage(const std::string &buffer)`
  - Purpose: Determine whether a newline is present to indicate a full message.
  - Behavior: Returns true if buffer contains `\n`.

- `extractNextMessage(std::string &buffer)`
  - Purpose: Get the next message up to the first `\n`, return the message without the trailing newline (and without trailing `\r`).
  - Behavior:
    - Finds the position of `\n`, then calls `extractMessage` to get a cleaned substring, erases the part (including newline) from the buffer, and returns the message.

- `extractMessage(const std::string &buffer, size_t pos)`
  - Purpose: Strip optional `\r` at `pos - 1` and return substring `[0, end)`.
  - Behavior: If char before `\n` is `\r`, excludes it from the returned message.

- `parseMessage(const std::string &message)`
  - Purpose: Tokenize a raw message into IRC tokens (`command`, params, and optional trailing).
  - Behavior:
    - Detects a trailing parameter introduced by `" :"` (space + colon).
    - If trailing present: splits into `prefix` (everything before `" :"`) and `trailing` (everything after).
    - Calls `tokenizePrefix` to split `prefix` on whitespace into tokens (first token is generally the command).
    - If trailing exists: pushes it as the last token in the returned list.
  - Note: This preserves the IRC semantics where a parameter prefixed by `:` may contain spaces.

- `tokenizePrefix(const std::string &prefix, std::list<std::string> &cmdList)`
  - Purpose: Simple whitespace tokenization of the non-trailing part of the message.
  - Behavior: Uses an `istringstream` to split on whitespace and append tokens in order.

---

Command execution and dispatch

- `executeCommand(int fd, std::list<std::string> cmdList)`
  - Purpose: Validate and execute a parsed command for a client.
  - Behavior:
    - If `cmdList` empty: return.
    - Extracts first token as `cmd`.
    - Verifies `cmd` is uppercase using `isUpperCase`. If not uppercase, calls `sendInvalidCommandError`.
    - Otherwise, normalizes to uppercase (defensive) and looks up `Client *client = clients[fd]`.
    - Calls `dispatchCommand(cmd, cmdList, client)` to hand off to the command handlers.
  - Notes: Commands must be uppercase (server enforces this policy).

- `sendInvalidCommandError(int fd, const std::string &cmd)`
  - Purpose: Inform the client that a command used incorrect casing or was invalidly formatted.
  - Behavior: Sends an `ERR_UNKNOWNCOMMAND` style reply (server-specific formulation) and logs a warning.

- `dispatchCommand(const std::string &cmd, std::list<std::string> cmdList, Client *client)`
  - Purpose: Route recognized commands to their specific handlers.
  - Behavior:
    - Uses a sequence of `if/else` checks for recognized commands: `PASS`, `NICK`, `USER`, `JOIN`, `PRIVMSG`, `PART`, `MODE`, `INVITE`, `NAMES`, `TOPIC`, `KICK`, `QUIT`, `PING`.
    - Each recognized command calls a corresponding handler (e.g., `handlePass(...)`).
    - Unknown commands are handled by `sendUnknownCommandError`.
  - Note: Handlers are in other compilation units; ensure their signatures match expected calls.

- `sendUnknownCommandError(Client *client, const std::string &cmd)`
  - Purpose: Send an error reply for commands that are not implemented.
  - Behavior: Builds an error message using the client's nickname (or `*` if not set) and logs a warning.

- `isUpperCase(const std::string &str)`
  - Purpose: Utility verifying that all characters in the provided string are uppercase.
  - Behavior: Iterates and ensures each character satisfies `std::isupper`.
  - Note: Used to enforce server policy that commands must be uppercase.

---

Channel cleanup helper

- `removeChannel(const std::string &channelName)`
  - Purpose: Delete a channel by name if it exists.
  - Behavior:
    - Finds the channel in `channels` map, deletes the `Channel *`, erases map entry, logs info.
  - Side effects: Deallocates `Channel` memory.

---

Signal handling

- `sigHandler(int sig)`
  - Purpose: Minimal signal handler for shutdown signals.
  - Behavior: Sets a `signal` flag (a server member) to true and logs a warning that a signal was received; does not perform heavy operations inside the handler.
  - Note: The server's main loop should check `signal` to perform a graceful shutdown.

---

Important implementation notes and best-practice hints
- Non-blocking sockets: Newly accepted sockets are set to non-blocking. All reads and writes must be prepared to handle EAGAIN/EWOULDBLOCK.
- `epoll` flags: Clients are registered with `EPOLLIN | EPOLLHUP | EPOLLERR`. The server handles HUP/ERR as disconnects.
- MSG_DONTWAIT vs non-blocking: `tryHandleHttpClient` uses `recv(..., MSG_DONTWAIT)` so it can probe immediately even if the fd is blocking or non-blocking; however the socket is set to non-blocking prior to heavy usage.
- Avoid SIGPIPE: `send` calls use `MSG_NOSIGNAL` or similar to prevent SIGPIPE on write to closed peers.
- processedFds: The `processedFds` set prevents multiple disconnect/cleanup runs for the same fd during a single event loop where multiple events for the same fd might be delivered. Be careful to clear or recreate this set appropriately across iterations; it is intended to be ephemeral per-loop.
- Memory management: `Client` and `Channel` are manually allocated and deleted. Ensure any new paths that remove clients or channels follow the same deletion semantics to avoid leaks or double deletes.
- Parsing: The server expects messages delimited by `\n`. `\r` immediately before `\n` is stripped. Trailing parameters beginning with `:` are preserved as the last token and may contain spaces.
- Command handlers: The `dispatchCommand` function delegates to handlers such as `handleNick` or `handleJoin`. Those handlers should expect the tokenization described here (first element is the command, final element may be a trailing parameter).

---

How to extend / test
- Adding a new IRC command:
  1. Implement the handler with signature matching other handlers (take a token list, `Client*`, and `Server*` as needed).
  2. Add an `else if (cmd == "MYCMD") handleMyCmd(cmdList, client, this);` branch in `dispatchCommand`.
  3. Unit test: create an integration test establishing a connection, send a message `MYCMD param1 :trailing param`, and assert appropriate server behavior.

- Testing HTTP probe behavior:
  - Connect a TCP client and immediately send `GET / HTTP/1.1\r\nHost: ...\r\n\r\n`. The server should respond with `200 OK` and close.
  - Connect but send nothing: the server should greet the client as IRC after no immediate data on accept.

- Testing disconnect and cleanup:
  - Join multiple channels, then close the connection. Verify the client is removed from channels and that empty channels are deleted.

- Observability:
  - Logs are produced on new connection, diagnostic warnings for accept/read errors, adding to epoll, and client disconnections. Use these logs while testing to trace lifecycle events.

---

If you need a sequence diagram or sample message flows (raw IRC examples, sample logs for acceptance, HTTP detection, and disconnect), tell me which scenario you'd like and I will add it to this README.
