# KVStore - In-Memory Key-Value Store with TTL

A thread-safe, in-memory Redis-like key-value store implemented in C++ with TTL (time-to-live) support, persistence, and both TCP and stdin interfaces.

## Features

### Must-Have Requirements ✅
- ✅ **Basic Commands**: SET, GET, DEL, KEYS (glob pattern matching), TTL
- ✅ **TTL Expiration**: Auto-expire with lazy deletion + periodic cleanup every 1 second
- ✅ **Thread-Safe**: All operations protected by mutexes for concurrent access
- ✅ **Protocol Support**: Both TCP socket (port configurable) and stdin interface
- ✅ **Persistence**: SAVE/LOAD commands with JSON snapshots
- ✅ **Statistics**: STATS command showing memory usage, key count, cleanup metrics

### Bonus Features ⭐
- ✅ **INCR/DECR**: Atomic integer operations
- ✅ **List Operations**: LPUSH, RPUSH, LPOP, RPOP, LLEN for list management
- ✅ **Pub/Sub**: PUBLISH, SUBSCRIBE, UNSUBSCRIBE for event messaging
- ✅ **LRU Eviction**: Automatic eviction when max keys limit is exceeded

## Project Structure

```
kvstore/
├── CMakeLists.txt          # Build configuration
├── include/
│   ├── kvstore.h          # Main KVStore class
│   ├── command_parser.h    # Command parsing and execution
│   └── persistence.h       # Persistence utilities
├── src/
│   ├── main.cpp           # Server (TCP + stdin modes)
│   ├── kvstore.cpp        # KVStore implementation
│   └── command_parser.cpp  # Command parser implementation
├── README.md              # This file
└── run.bat               # Run script for Windows (NEW!)
```

## Building

### Prerequisites
- CMake 3.10 or higher
- C++17 compatible compiler
- Windows: Visual Studio or MinGW
- Linux/Mac: GCC/Clang

### Build Steps

#### Using run.bat (Windows)
```bash
run.bat
```
This will automatically:
1. Create build directory
2. Run CMake
3. Compile the project
4. Run the server in stdin mode

#### Manual Build

```bash
# Create and enter build directory
mkdir build
cd build

# Generate build files
cmake ..

# Build the project
cmake --build . --config Release

# Run the server
./bin/kvstore stdin
# or for TCP mode
./bin/kvstore tcp 6379
```

## Usage

### Stdin Mode (Default)
```bash
kvstore stdin
```
Input commands line by line:
```
SET user:1 '{"name":"Alice"}' EX 300
GET user:1
TTL user:1
KEYS user:*
```

### TCP Mode
```bash
kvstore tcp 6379
```
Then connect from another terminal:
```bash
# On Linux/Mac
nc localhost 6379

# Or use telnet
telnet localhost 6379

# Or write a Python client
python3
>>> import socket
>>> s = socket.socket()
>>> s.connect(('localhost', 6379))
>>> s.send(b'SET mykey "hello"\n')
>>> print(s.recv(1024).decode())
OK
```

## Command Reference

### Basic Operations

```bash
SET key value [EX seconds]    # Set key with optional TTL
GET key                       # Get value of key
DEL key [key ...]            # Delete one or more keys
KEYS pattern                 # Get keys matching glob pattern (* = any)
TTL key                      # Get remaining TTL (-1 no expiry, -2 not found)
```

### Integer Operations (BONUS)

```bash
INCR key                     # Increment integer value
DECR key                     # Decrement integer value
```

### List Operations (BONUS)

```bash
LPUSH key val [val ...]      # Push values to head
RPUSH key val [val ...]      # Push values to tail
LPOP key                     # Pop from head
RPOP key                     # Pop from tail
LLEN key                     # Get list length
```

### Pub/Sub (BONUS)

```bash
PUBLISH channel message      # Publish to channel
SUBSCRIBE channel            # Subscribe to channel
UNSUBSCRIBE channel          # Unsubscribe from channel
```

### Persistence

```bash
SAVE filename                # Save snapshot to JSON file
LOAD filename                # Load snapshot from JSON file
```

### Utilities

```bash
STATS                        # Display statistics
HELP                        # Show all commands
QUIT / EXIT                 # Close connection
```

## Example Usage

```bash
$ kvstore stdin

=== KVStore Started (STDIN Mode) ===
Type 'HELP' for commands

# Basic SET/GET
SET counter 0
OK
GET counter
0

# TTL with expiration
SET session:123 "active" EX 60
OK
TTL session:123
59
TTL session:123
58

# Pattern matching
SET user:1 "Alice"
OK
SET user:2 "Bob"
OK
KEYS user:*
user:1
user:2

# Integer operations
INCR counter
1
INCR counter
2
DECR counter
1

# List operations
LPUSH mylist a b c
3
RPUSH mylist d e
5
LPOP mylist
c
RPOP mylist
e
LLEN mylist
3

# Persistence
SAVE snapshot.json
OK
LOAD snapshot.json
OK

# Statistics
STATS
Total keys: 5
Expired keys cleaned: 2
Estimated memory usage: 0.5 KB
Max keys limit: 10000
```

## Performance Characteristics

- **SET/GET/DEL**: O(1) average case
- **KEYS**: O(n) where n is total keys
- **LPUSH/RPUSH**: O(1)
- **LPOP/RPOP**: O(1)
- **INCR/DECR**: O(1)
- **Expiration Cleanup**: Background thread runs every 1 second

## Thread Safety

All operations use mutex locking:
- `data_mutex`: Protects main key-value store
- `expiry_mutex`: Protects expiry queue
- Background cleanup thread runs periodically

Multiple clients can safely access the store concurrently.

## Limitations

- In-memory only (no disk-based persistence beyond snapshots)
- Single-threaded command execution per TCP connection
- Simple glob pattern matching (* and ? only)
- LRU eviction is basic (no access tracking across calls)
- Pub/Sub is topic-based only (no pattern subscriptions)

## Extensibility

Easy to add new commands by:
1. Adding handler method in `CommandParser` class
2. Extending kvstore.h with new KVStore method
3. Updating help text

## Testing

Sample test commands (input):
```
SET user:1 '{"name":"Alice"}' EX 300
GET user:1
TTL user:1
SET counter 0
KEYS user:*
DEL user:1
GET user:1
```

Expected output:
```
OK
{"name":"Alice"}
299
OK
user:1
1
(nil)
```

## Author & License

Built for uTrade Solutions Campus Hiring 2026 - Set C Challenge

## Credits

Implementation features:
- C++17 standard library
- Cross-platform Windows/Linux support
- Clean object-oriented design
- Memory-efficient storage
- Concurrent client handling
