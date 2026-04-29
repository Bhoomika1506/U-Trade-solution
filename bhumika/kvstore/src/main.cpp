#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
typedef int SOCKET;
#endif

#include "kvstore.h"
#include "command_parser.h"

class Server {
private:
    std::shared_ptr<KVStore> store;
    std::shared_ptr<CommandParser> parser;
    int port;
    bool use_tcp;
    SOCKET server_socket = INVALID_SOCKET;

public:
    Server(int port, bool use_tcp = true) : port(port), use_tcp(use_tcp) {
        store = std::make_shared<KVStore>();
        parser = std::make_shared<CommandParser>(store);
    }

    ~Server() {
        if (server_socket != INVALID_SOCKET) {
            closesocket(server_socket);
        }
#ifdef _WIN32
        WSACleanup();
#endif
    }

    void run() {
        if (use_tcp) {
            runTcpServer();
        } else {
            runStdinServer();
        }
    }

private:
    void printHelp() {
        std::cout << R"(
=== KVStore - Redis-like In-Memory Key-Value Store ===
Available Commands:

Basic Operations:
  SET key value [EX seconds]        - Set key with optional TTL
  GET key                           - Get value
  DEL key [key ...]                 - Delete keys
  KEYS pattern                      - List keys matching pattern (* supported)
  TTL key                           - Get remaining TTL

Integer Operations:
  INCR key                          - Increment integer
  DECR key                          - Decrement integer

List Operations:
  LPUSH key val [val ...]           - Push to head
  RPUSH key val [val ...]           - Push to tail
  LPOP key                          - Pop from head
  RPOP key                          - Pop from tail
  LLEN key                          - Get list length

Pub/Sub:
  PUBLISH channel message           - Publish message
  SUBSCRIBE channel                 - Subscribe to channel
  UNSUBSCRIBE channel               - Unsubscribe

Data Management:
  SAVE filename                     - Save to JSON
  LOAD filename                     - Load from JSON
  STATS                             - Show statistics

Other:
  HELP                              - Show this help
  QUIT/EXIT                         - Exit

Examples:
  SET mykey "Hello" EX 300
  GET mykey
  KEYS user:*
  INCR counter
  LPUSH mylist a b c
)";
    }

    void runStdinServer() {
        std::cout << "\n=== KVStore Started (STDIN Mode) ===\n";
        std::cout << "Type 'HELP' for commands\n\n";

        std::string line;
        while (std::getline(std::cin, line)) {
            if (line.empty()) continue;
            
            std::string upper_line = line;
            std::transform(upper_line.begin(), upper_line.end(), upper_line.begin(), ::toupper);
            
            if (upper_line == "EXIT" || upper_line == "QUIT") {
                std::cout << "Goodbye!\n";
                break;
            }
            if (upper_line == "HELP") {
                printHelp();
                continue;
            }

            std::string result = parser->execute(line);
            std::cout << result;
        }
    }

    void runTcpServer() {
#ifdef _WIN32
        WSADATA wsa_data;
        if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
            std::cerr << "WSAStartup failed\n";
            return;
        }
#endif

        server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (server_socket == INVALID_SOCKET) {
            std::cerr << "Socket creation failed\n";
            return;
        }

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        server_addr.sin_port = htons(port);

        int reuse = 1;
        if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse)) < 0) {
            std::cerr << "setsockopt failed\n";
        }

        if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            std::cerr << "Bind failed on port " << port << "\n";
            closesocket(server_socket);
            return;
        }

        if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << "Listen failed\n";
            closesocket(server_socket);
            return;
        }

        std::cout << "\n=== KVStore Server Started (TCP Mode) ===\n";
        std::cout << "Listening on 127.0.0.1:" << port << "\n";
        std::cout << "Clients can connect and send commands...\n";

        while (true) {
            sockaddr_in client_addr{};
            socklen_t client_addr_len = sizeof(client_addr);
            
            SOCKET client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
            if (client_socket == INVALID_SOCKET) {
                std::cerr << "Accept failed\n";
                continue;
            }

            // Handle client in a new thread
            std::thread(&Server::handleClient, this, client_socket).detach();
        }
    }

    void handleClient(SOCKET client_socket) {
        char buffer[4096];
        int client_id = (int)(uintptr_t)&client_socket;

        while (true) {
            int recv_size = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
            if (recv_size <= 0) {
                break;
            }

            buffer[recv_size] = '\0';
            std::string command(buffer);

            // Handle multiple commands separated by newlines
            size_t pos = 0;
            while (pos < command.length()) {
                size_t newline_pos = command.find('\n', pos);
                if (newline_pos == std::string::npos) newline_pos = command.length();

                std::string cmd = command.substr(pos, newline_pos - pos);
                pos = newline_pos + 1;

                if (cmd.empty()) continue;

                // Check for quit
                std::string upper_cmd = cmd;
                std::transform(upper_cmd.begin(), upper_cmd.end(), upper_cmd.begin(), ::toupper);
                
                if (upper_cmd == "QUIT" || upper_cmd == "EXIT") {
                    closesocket(client_socket);
                    return;
                }

                std::string result = parser->execute(cmd, std::to_string(client_id));
                
                if (send(client_socket, result.c_str(), result.length(), 0) == SOCKET_ERROR) {
                    closesocket(client_socket);
                    return;
                }
            }
        }

        closesocket(client_socket);
    }
};

void printUsage(const char* program) {
    std::cout << "Usage: " << program << " [mode] [port]\n";
    std::cout << "  mode: 'stdin' (default) or 'tcp'\n";
    std::cout << "  port: TCP port (default 6379, only for tcp mode)\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << program << " stdin\n";
    std::cout << "  " << program << " tcp 6379\n";
}

int main(int argc, char* argv[]) {
    bool use_tcp = false;
    int port = 6379;

    if (argc > 1) {
        std::string mode = argv[1];
        std::transform(mode.begin(), mode.end(), mode.begin(), ::tolower);
        
        if (mode == "help" || mode == "-h" || mode == "--help") {
            printUsage(argv[0]);
            return 0;
        }
        
        if (mode == "tcp") {
            use_tcp = true;
            if (argc > 2) {
                try {
                    port = std::stoi(argv[2]);
                } catch (...) {
                    std::cerr << "Invalid port number\n";
                    return 1;
                }
            }
        } else if (mode != "stdin") {
            std::cerr << "Invalid mode: " << mode << "\n";
            printUsage(argv[0]);
            return 1;
        }
    }

    try {
        Server server(port, use_tcp);
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
