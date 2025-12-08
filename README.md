# `elkirc`


An ultra-lightweight, ELKS-first IRC client with broad OS support.

## Overview

`elkirc` is a minimal, portable IRC client designed for resource-constrained environments while remaining fully functional on modern platforms.

**Key features:**
- **ELKS-native:** Primary target is ELKS (8086/286 with minimal RAM)
- **Minimal footprint:** ~1.3 KB RAM for buffers + ~100–200 KB with libc
- **Portable:** Builds on modern gcc (Linux, BSD, macOS, ARM, x86)
- **Non-blocking I/O:** Uses `select()` for concurrent socket + stdin handling
- **Zero dependencies:** No external libraries beyond POSIX/BSD sockets
- **Standard IRC:** Supports `/nick`, `/join`, `/msg`, `/raw`, and auto-PING responses
- **Color support:** Optional ANSI color output for better readability (use `-c` flag)


## Building

For modern platforms (Linux, macOS, BSD):
```shell
make
```

For ELKS:
```
make ELKS=1
```

This sets `-D__ELKS__` to use ELKS-specific headers instead of POSIX equivalents.


## Usage

```shell
elkirc [-d|--debug] [-c|--color] [-s server] [-p port] [-n nick]
```

**Options:**
- `-d, --debug` - Enable debug mode (show PING messages)
- `-c, --color` - Enable color output for better readability
- `-s server` - IRC server hostname (optional, can connect later with `/connect`)
- `-p port` - IRC server port (optional, defaults to 6667 if server is specified)
- `-n nick` - Your nickname (optional, defaults to system username if not specified)

**Examples:**
```shell
# Connect immediately with all options
elkirc -d -c -s irc.libera.chat -p 6667 -n alice

# Connect with minimal options (uses default port 6667 and system username)
elkirc -s irc.libera.chat

# Start without connecting (connect later with /connect)
elkirc -n alice

# Start with system username, no connection
elkirc
```

**Behavior:**
- If `server` and `port` are not provided, elkirc starts without connecting. Use `/connect <server> [port]` to connect later.
- If `nick` is not provided, elkirc uses your system username (from `$USER` or `getpwuid()`).
- Once connected, type messages directly to send to the current target (channel or user).
- Use commands (see below) prefixed with `/`.
- Press Ctrl+C, `/quit` (disconnect) or `/exit` (quit) to exit.


## Commands

Inside `elkirc`, you can use the following commands:

- `/connect <server> [port]` or `/c <server> [port]` — Connect to an IRC server (port defaults to 6667)
- `/nick <newnick>` or `/n <newnick>` — Change your nickname
- `/join <#channel|nick>` or `/j <#channel|nick>` — Join a channel or switch to private messages with a user
- `/leave [target]` or `/l [target]` or `/part [target]` or `/p [target]` — Leave a channel or private conversation (leaves current target if none specified)
- `/msg <nick> <message>` — Send a direct message to a user or bot
- `/raw <command>` — Send raw IRC commands (e.g., /raw MODE #channel +o alice)
- `/quit` or `/q` — Disconnect from server (sends QUIT but doesn't exit elkirc)
- `/exit` or `/e` — Exit elkirc (sends QUIT if connected, then exits)

**Note:** When leaving a channel or disconnecting, the target automatically returns to your nick (instead of showing `[*no-target]`).


## Color Support

When enabled with `-c` or `--color`, elkirc uses ANSI color codes to enhance readability:

- **Prompt**: Channel/nick in orange, brackets in bold (e.g., `[#channel]`)
- **User messages**: Your own messages appear in bold
- **Channel messages**: Nicks in blue, brackets in bold (e.g., `<nick> message`)
- **NOTICE messages**: Content after `:***` in purple (prefix hidden)
- **Server numeric replies**: Messages matching `:server.domain XXX nick ` pattern:
  - **0XX/1XX/2XX**: Content after "nick " in blue (prefix in grey if debug enabled)
  - **3XX (MOTD/NAMES)**: Content after "nick " in green (prefix in grey if debug enabled)
  - **4XX/5XX (errors)**: Content after "nick " in red (prefix in grey if debug enabled)
  - Prefix `:server.domain XXX nick ` is only shown when `-d/--debug` is enabled
- **JOIN messages**: Entire message in green
- **Other server messages**: Messages starting with `:` in grey

## Design Notes

- **Fixed buffers**: All I/O uses 256-byte buffers (defined in `elkirc.h`) to avoid dynamic allocation
- **ELKS compatibility**: Conditional includes for ELKS vs. modern POSIX headers; no malloc/free in hot paths
- **Non-blocking**: Server messages arrive immediately without blocking user input
- **PING/PONG**: Automatic server PING handling to keep connection alive (hidden by default, use `-d` to show)


## Limitations

- IPv4 only (no IPv6)
- No SSL/TLS (not feasible on ELKS)
- No channel history or backlog
- No multi-server support
- Basic RFC 1459 IRC only (no modern extensions like SASL)


## License

See [LICENSE](LICENSE) file.