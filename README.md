# IRC Server (RFC 2812) — `ircserv`

A small, standards-inspired IRC server written in C++98.

This project implements a practical subset of **RFC 2812 (Internet Relay Chat: Client Protocol)**, focusing on the core registration flow, channel management, messaging, and the most common operator actions.

It also includes an optional **bonus bot** (`cisor_bot`) that connects as an IRC client and plays Rock–Paper–Scissors.

---

## Features

- **RFC 2812-style client protocol (subset)**
  - Registration: `PASS`, `NICK`, `USER`
  - Messaging: `PRIVMSG`, `PING`/`PONG`, `QUIT`
  - Channels: `JOIN`, `PART`, `TOPIC`, `NAMES`
  - Moderation: `INVITE`, `KICK`, `MODE`
- **Channel modes implemented** (see “Channel Modes”)
- **Multiple targets** supported in `JOIN` and `PRIVMSG` (comma-separated)
- **Single-process, event-driven I/O** using `epoll` (non-blocking sockets)
- **Graceful cleanup** of clients/channels on disconnect
- **Friendly behavior for accidental HTTP clients** (returns a small HTTP response if you open the port in a browser)

---

## Build

### Requirements

- Linux (uses `epoll`)
- `c++` compiler with C++98 support
- `make`

### Compile

```bash
make
```

This produces the server binary: `./ircserv`

### Bonus (bot)

```bash
make bonus
```

This produces the bot binary: `./bot/cisor_bot`

> Note: the default `Makefile` enables AddressSanitizer (`-fsanitize=address`) and debug symbols (`-g`). If you want a release-like build, adjust `CFLAGS` in `Makefile`.

---

## Run

### Start the server

```bash
./ircserv <port> <password>
```

Example:

```bash
./ircserv 6667 supersecret
```

Constraints enforced by the server:

- Port must be in **[1024, 65535]**
- Password must not contain spaces or non-printable characters

---

## Connect (quick test)

### Using Netcat (raw IRC)

```bash
nc 127.0.0.1 6667
```

Then type the RFC-style registration sequence:

```text
PASS supersecret

NICK alice

USER alice 0 * :Alice Example

JOIN #lobby

PRIVMSG #lobby :hello from netcat!

```

Important behavioral notes in this implementation:

- Commands are expected to be **uppercase** (the server rejects lowercase commands).
- Registration order is enforced:
  - `PASS` must come **before** `NICK`/`USER`
  - `NICK` must come **before** `USER`

### Using an IRC client

You can connect with any normal IRC client (e.g., irssi, WeeChat, HexChat) by setting:

- Server/Host: `127.0.0.1`
- Port: `6667` (or your chosen port)
- Password: the password passed to `ircserv`

---

## Supported Commands (RFC 2812 subset)

The server implements the following commands:

### Registration

- `PASS <password>`
  - Must be sent before `NICK`/`USER`
- `NICK <nickname>`
  - Nicknames are case-insensitive for uniqueness checks
- `USER <username> 0 * :<realname>`
  - The `mode` parameter must be `0` in this implementation

### Channels

- `JOIN <#channel>[,<#channel>...] [<key>[,<key>...]]`
  - Channel names must start with `#`
- `PART <#channel>[,<#channel>...] [:message]`
- `TOPIC <#channel> [:<topic>]`
  - Without a topic parameter, shows current topic
- `NAMES [<#channel>[,<#channel>...]]`

### Messaging

- `PRIVMSG <target>[,<target>...] :<text>`
  - `target` can be a nickname or a `#channel`
  - Messages longer than the IRC limit (512 incl. CRLF) are rejected
- `PING <token>`
  - Replies with `PONG`
- `QUIT [:message]`

### Channel operator / moderation

- `INVITE <#channel> <nick>`
  - On invite-only channels, only operators may invite
- `KICK <#channel> <nick> [:comment]`
- `MODE <#channel> <modes> [mode params...]`

---

## Channel Modes

The `MODE` command supports the following channel modes:

- `+i` / `-i` — invite-only
- `+t` / `-t` — restrict topic changes to channel operators
- `+k <key>` / `-k` — channel key (password)
- `+l <limit>` / `-l` — user limit
- `+o <nick>` / `-o <nick>` — grant/remove operator status

Notes:

- Most `MODE` changes require the sender to be a channel operator.
- `JOIN` enforces `+i`, `+k`, and `+l`.

---

## Bonus: `cisor_bot` (Rock–Paper–Scissors)

The bonus program is an IRC client bot that connects to the server and responds to `PRIVMSG`.

### Run the bot

1) Start the server:

```bash
./ircserv 6667 supersecret
```

2) Start the bot:

```bash
./bot/cisor_bot 127.0.0.1 6667 supersecret
```

The bot will authenticate using:

- `NICK cisor`
- `USER bot 0 * :cisor bot`

### Bot commands (via PRIVMSG)

Send messages to `cisor`:

- `help` — show instructions
- `rock` / `paper` / `cisor` (or `scissors`) — play a round
- `score` / `scoreboard` — show scores
- Multiplayer rooms:
  - `room create`
  - `room join`
  - `room leave`
  - `room status`
  - `play <move>` (multiplayer move)

---

## Project Layout

- `srcs/` — server sources (network loop, parsing, server core)
- `srcs/commands/` — IRC command handlers
- `includes/` — server headers
- `bot/` — bonus bot sources and headers
- `ft_irc.pdf` — project/spec reference (included in repo)

---

## Troubleshooting

- **“Commands must be uppercase”**
  - Use `PASS`, not `pass`.
- **Can’t register / “out of order”**
  - Ensure you send `PASS` first, then `NICK`, then `USER`.
- **Can’t join a channel**
  - Check if the channel is `+i` (invite-only), `+k` (key required), or `+l` (user limit).
- **Port issues**
  - Use a non-privileged port (>= 1024), and ensure the port isn’t already in use.

---

## Resources

### RFCs / protocol references

- RFC 2812 — Internet Relay Chat: Client Protocol
  - https://www.rfc-editor.org/rfc/rfc2812
- RFC 2811 — Internet Relay Chat: Channel Management
  - https://www.rfc-editor.org/rfc/rfc2811
- RFC 2813 — Internet Relay Chat: Server Protocol
  - https://www.rfc-editor.org/rfc/rfc2813
- RFC 1459 — The original IRC protocol specification (historical)
  - https://www.rfc-editor.org/rfc/rfc1459

### Modern IRC documentation

- ircdocs (modern, practical IRC documentation)
  - https://modern.ircdocs.horse/

### Useful tooling

- irssi
  - https://irssi.org/
- WeeChat
  - https://weechat.org/
- `nc` (netcat) for raw protocol testing
  - https://man7.org/linux/man-pages/man1/nc.1.html

### Linux networking

- `epoll(7)`
  - https://man7.org/linux/man-pages/man7/epoll.7.html

---
