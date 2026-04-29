KVStore - In-Memory Key-Value Store with TTL

A thread-safe, in-memory key-value store inspired by Redis, built in C++. It supports TTL (time-to-live), persistence, and can run through both TCP sockets and standard input.

Main Features
Core Requirements
Supports basic commands such as SET, GET, DEL, KEYS, TTL
Keys can expire automatically using TTL
Expired keys are removed through lazy deletion and a cleanup task running every second
Thread-safe design using mutex locks
Works in two modes: TCP server and stdin
Data can be saved and loaded using JSON snapshots
Includes a STATS command for memory usage, key count, and cleanup info
Extra Features
INCR / DECR for numeric values
List operations: LPUSH, RPUSH, LPOP, RPOP, LLEN
Pub/Sub messaging with PUBLISH, SUBSCRIBE, UNSUBSCRIBE
Basic LRU eviction when key limit is reached
Project Layout
kvstore/
├── CMakeLists.txt
├── include/
│   ├── kvstore.h
│   ├── command_parser.h
│   └── persistence.h
├── src/
│   ├── main.cpp
│   ├── kvstore.cpp
│   └── command_parser.cpp
├── README.md
└── run.bat

How to Build
Requirements
CMake 3.10+
C++17 compiler
Visual Studio / MinGW (Windows)
GCC / Clang (Linux/Mac)
Quick Run (Windows)
run.bat

This script will:

Create the build folder
Run CMake
Compile the code
Start the program in stdin mode
Manual Build
mkdir build
cd build
cmake ..
cmake --build . --config Release
./bin/kvstore stdin

For TCP mode:

./bin/kvstore tcp 6379

How to Use
Stdin Mode
kvstore stdin

Example:

SET user:1 {"name":"Alice"} EX 300
GET user:1
TTL user:1
KEYS user:*
TCP Mode
kvstore tcp 6379

Then connect using tools like:

nc localhost 6379
telnet localhost 6379

Supported Commands
Basic Commands
SET key value [EX seconds]
GET key
DEL key [key ...]
KEYS pattern
TTL key
Number Commands
INCR key
DECR key
List Commands
LPUSH key val [val ...]
RPUSH key val [val ...]
LPOP key
RPOP key
LLEN key
Pub/Sub
PUBLISH channel message
SUBSCRIBE channel
UNSUBSCRIBE channel
Save / Load
SAVE filename
LOAD filename
Utility
STATS
HELP
QUIT / EXIT

Example Session
SET counter 0
GET counter
INCR counter
SET session:123 active EX 60
TTL session:123
SAVE snapshot.json
STATS

Possible output:

OK
0
1
OK
59
OK
Total keys: 2
Estimated memory usage: 0.5 KB

Performance
SET / GET / DEL → O(1) average
KEYS → O(n)
LPUSH / RPUSH / POP → O(1)
INCR / DECR → O(1)
Cleanup thread runs every second
Thread Safety

The project uses separate mutexes for:

Main data store
Expiry management

This allows multiple clients to use the store safely at the same time.

Current Limitations
Data is stored only in memory unless manually saved
One command thread per TCP client
Pattern matching supports basic wildcards only
LRU eviction is simple
Pub/Sub supports channels only
Future Improvements

New commands can be added easily by:

Creating a handler in CommandParser
Extending the KVStore class
Updating the help section
Project Purpose

Built for the uTrade Solutions Campus Hiring 2026 - Set C Challenge.
