#include "command_parser.h"
#include <sstream>
#include <algorithm>
#include <iostream>

CommandParser::CommandParser(std::shared_ptr<KVStore> store) : store(store) {}

std::vector<std::string> CommandParser::tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    bool in_quotes = false;
    std::string current;

    for (size_t i = 0; i < input.length(); i++) {
        char c = input[i];
        if ((c == '\'' || c == '"') && (i == 0 || input[i-1] != '\\')) {
            in_quotes = !in_quotes;
        } else if ((c == ' ' || c == '\n' || c == '\t') && !in_quotes) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }

    if (!current.empty()) {
        tokens.push_back(current);
    }

    return tokens;
}

std::string CommandParser::toUpperCase(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}

std::string CommandParser::execute(const std::string& command_line, const std::string& client_id_param) {
    if (client_id_param != "") {
        client_id = client_id_param;
    }
    
    auto tokens = tokenize(command_line);
    if (tokens.empty()) {
        return "ERROR: empty command\n";
    }

    std::string command = toUpperCase(tokens[0]);

    if (command == "SET") {
        return handleSet(tokens);
    } else if (command == "GET") {
        return handleGet(tokens);
    } else if (command == "DEL") {
        return handleDel(tokens);
    } else if (command == "KEYS") {
        return handleKeys(tokens);
    } else if (command == "TTL") {
        return handleTtl(tokens);
    } else if (command == "STATS") {
        return handleStats(tokens);
    } else if (command == "SAVE") {
        return handleSave(tokens);
    } else if (command == "LOAD") {
        return handleLoad(tokens);
    } else if (command == "INCR") {
        return handleIncr(tokens);
    } else if (command == "DECR") {
        return handleDecr(tokens);
    } else if (command == "LPUSH") {
        return handleLpush(tokens);
    } else if (command == "RPUSH") {
        return handleRpush(tokens);
    } else if (command == "LPOP") {
        return handleLpop(tokens);
    } else if (command == "RPOP") {
        return handleRpop(tokens);
    } else if (command == "LLEN") {
        return handleLlen(tokens);
    } else if (command == "PUBLISH") {
        return handlePublish(tokens);
    } else if (command == "SUBSCRIBE") {
        return handleSubscribe(tokens);
    } else if (command == "UNSUBSCRIBE") {
        return handleUnsubscribe(tokens);
    } else if (command == "HELP") {
        return handleHelp(tokens);
    } else {
        return "ERROR: unknown command\n";
    }
}

std::string CommandParser::handleSet(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        return "ERROR: wrong number of arguments for 'SET'\n";
    }

    std::string key = args[1];
    std::string value = args[2];
    long long ttl_seconds = -1;

    // Parse EX option
    for (size_t i = 3; i < args.size(); i++) {
        if (toUpperCase(args[i]) == "EX" && i + 1 < args.size()) {
            try {
                ttl_seconds = std::stoll(args[i + 1]);
                i++;
            } catch (...) {
                return "ERROR: invalid TTL value\n";
            }
        }
    }

    if (store->set(key, value, ttl_seconds)) {
        return "OK\n";
    }
    return "ERROR: SET failed\n";
}

std::string CommandParser::handleGet(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        return "ERROR: wrong number of arguments for 'GET'\n";
    }

    std::string result = store->get(args[1]);
    return result + "\n";
}

std::string CommandParser::handleDel(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        return "ERROR: wrong number of arguments for 'DEL'\n";
    }

    int count = 0;
    for (size_t i = 1; i < args.size(); i++) {
        if (store->del(args[i])) {
            count++;
        }
    }

    return std::to_string(count) + "\n";
}

std::string CommandParser::handleKeys(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        return "ERROR: wrong number of arguments for 'KEYS'\n";
    }

    std::vector<std::string> keys = store->keys(args[1]);
    
    if (keys.empty()) {
        return "(empty list)\n";
    }

    std::string result;
    for (const auto& key : keys) {
        result += key + "\n";
    }
    return result;
}

std::string CommandParser::handleTtl(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        return "ERROR: wrong number of arguments for 'TTL'\n";
    }

    long long ttl = store->ttl(args[1]);
    return std::to_string(ttl) + "\n";
}

std::string CommandParser::handleStats(const std::vector<std::string>& args) {
    return store->get_stats();
}

std::string CommandParser::handleSave(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        return "ERROR: wrong number of arguments for 'SAVE'\n";
    }

    if (store->save(args[1])) {
        return "OK\n";
    }
    return "ERROR: SAVE failed\n";
}

std::string CommandParser::handleLoad(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        return "ERROR: wrong number of arguments for 'LOAD'\n";
    }

    if (store->load(args[1])) {
        return "OK\n";
    }
    return "ERROR: LOAD failed\n";
}

std::string CommandParser::handleIncr(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        return "ERROR: wrong number of arguments for 'INCR'\n";
    }

    long long result = 0;
    if (store->incr(args[1], result)) {
        return std::to_string(result) + "\n";
    }
    return "ERROR: INCR failed\n";
}

std::string CommandParser::handleDecr(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        return "ERROR: wrong number of arguments for 'DECR'\n";
    }

    long long result = 0;
    if (store->decr(args[1], result)) {
        return std::to_string(result) + "\n";
    }
    return "ERROR: DECR failed\n";
}

std::string CommandParser::handleLpush(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        return "ERROR: wrong number of arguments for 'LPUSH'\n";
    }

    std::string key = args[1];
    for (size_t i = 2; i < args.size(); i++) {
        store->lpush(key, args[i]);
    }

    long long len = store->llen(key);
    return std::to_string(len) + "\n";
}

std::string CommandParser::handleRpush(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        return "ERROR: wrong number of arguments for 'RPUSH'\n";
    }

    std::string key = args[1];
    for (size_t i = 2; i < args.size(); i++) {
        store->rpush(key, args[i]);
    }

    long long len = store->llen(key);
    return std::to_string(len) + "\n";
}

std::string CommandParser::handleLpop(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        return "ERROR: wrong number of arguments for 'LPOP'\n";
    }

    std::string result = store->lpop(args[1]);
    return result + "\n";
}

std::string CommandParser::handleRpop(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        return "ERROR: wrong number of arguments for 'RPOP'\n";
    }

    std::string result = store->rpop(args[1]);
    return result + "\n";
}

std::string CommandParser::handleLlen(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        return "ERROR: wrong number of arguments for 'LLEN'\n";
    }

    long long len = store->llen(args[1]);
    return std::to_string(len) + "\n";
}

std::string CommandParser::handlePublish(const std::vector<std::string>& args) {
    if (args.size() != 3) {
        return "ERROR: wrong number of arguments for 'PUBLISH'\n";
    }

    std::string channel = args[1];
    std::string message = args[2];
    
    auto subscribers = store->get_subscribers(channel);
    if (store->publish(channel, message)) {
        return std::to_string(subscribers.size()) + "\n";
    }
    return "0\n";
}

std::string CommandParser::handleSubscribe(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        return "ERROR: wrong number of arguments for 'SUBSCRIBE'\n";
    }

    std::string channel = args[1];
    if (store->subscribe(channel, client_id.empty() ? "client1" : client_id)) {
        return "OK\n";
    }
    return "ERROR: SUBSCRIBE failed\n";
}

std::string CommandParser::handleUnsubscribe(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        return "ERROR: wrong number of arguments for 'UNSUBSCRIBE'\n";
    }

    std::string channel = args[1];
    if (store->unsubscribe(channel, client_id.empty() ? "client1" : client_id)) {
        return "OK\n";
    }
    return "ERROR: UNSUBSCRIBE failed\n";
}

std::string CommandParser::handleHelp(const std::vector<std::string>& args) {
    return R"(
KVStore Commands:
  SET key value [EX seconds]        - Set a key with optional TTL
  GET key                           - Get value of key
  DEL key [key ...]                 - Delete one or more keys
  KEYS pattern                      - Get all keys matching pattern (* and ? supported)
  TTL key                           - Get remaining TTL in seconds (-1 no expiry, -2 not found)
  INCR key                          - Increment integer value
  DECR key                          - Decrement integer value
  LPUSH key value [value ...]       - Push values to head of list
  RPUSH key value [value ...]       - Push values to tail of list
  LPOP key                          - Pop from head of list
  RPOP key                          - Pop from tail of list
  LLEN key                          - Get list length
  PUBLISH channel message           - Publish message to channel
  SUBSCRIBE channel                 - Subscribe to channel
  UNSUBSCRIBE channel               - Unsubscribe from channel
  SAVE filename                     - Save snapshot to JSON file
  LOAD filename                     - Load snapshot from JSON file
  STATS                             - Show store statistics
  HELP                              - Show this help message
)";
}
