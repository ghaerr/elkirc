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
elkirc <server> <port> <nick>
```

Example:
```shell
elkirc irc.libera.chat 6667 alice
```

Once connected:
- Type messages directly to send to the current target (channel or user)
- Use commands (see below) prefixed with /
- Press Ctrl+C, /quit or /exit to quit


## Commands

Inside `elkirc`, you can use the following commands:

- `/nick <newnick>` - Change your nickname
- `/join <#channel|nick>` — Join a channel or switch to private messages with a user
- `/msg <nick> <message>` — Send a direct message to a user or bot
- `/raw <command>` — Send raw IRC commands (e.g., /raw MODE #channel +o alice)
- `/quit` — Send QUIT and exit gracefully
- `/exit` — Exit without sending QUIT


## Design Notes

- **Fixed buffers**: All I/O uses 256-byte buffers (defined in `elkirc.h`) to avoid dynamic allocation
- **ELKS compatibility**: Conditional includes for ELKS vs. modern POSIX headers; no malloc/free in hot paths
- **Non-blocking**: Server messages arrive immediately without blocking user input
- **PING/PONG**: Automatic server PING handling to keep connection alive


## Limitations

- IPv4 only (no IPv6)
- No SSL/TLS (not feasible on ELKS)
- No channel history or backlog
- No multi-server support
- Basic RFC 1459 IRC only (no modern extensions like SASL)


## License

See [LICENSE](LICENSE) file.