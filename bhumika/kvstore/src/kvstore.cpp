#include "kvstore.h"
#include <iostream>
#include <algorithm>
#include <thread>
#include <regex>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cmath>

KVStore::KVStore(size_t max_keys) : max_keys(max_keys) {
    // Start periodic cleanup thread
    std::thread cleanup_thread([this]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            cleanup();
        }
    });
    cleanup_thread.detach();
}

KVStore::~KVStore() = default;

long long KVStore::getCurrentTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

bool KVStore::patternMatch(const std::string& str, const std::string& pattern) {
    // Convert glob pattern to regex
    std::string regex_pattern = pattern;
    std::string escaped;
    for (size_t i = 0; i < regex_pattern.length(); i++) {
        char c = regex_pattern[i];
        if (c == '*') {
            escaped += ".*";
        } else if (c == '?' || c == '.' || c == '+' || c == '^' || c == '$' ||
                   c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}' || c == '|' || c == '\\') {
            escaped += '\\';
            escaped += c;
        } else {
            escaped += c;
        }
    }
    try {
        std::regex re("^" + escaped + "$");
        return std::regex_match(str, re);
    } catch (...) {
        return false;
    }
}

void KVStore::removeExpired(const std::string& key) {
    std::lock_guard<std::mutex> lock(data_mutex);
    auto it = data.find(key);
    if (it != data.end() && it->second.expiry_time != -1 && it->second.expiry_time < getCurrentTimeMs()) {
        data.erase(it);
        stats_expired_cleaned++;
    }
}

bool KVStore::set(const std::string& key, const std::string& value, long long ttl_seconds) {
    std::lock_guard<std::mutex> lock(data_mutex);

    // Check LRU eviction
    if (data.size() >= max_keys && data.find(key) == data.end()) {
        lruEvict();
    }

    long long expiry_time = -1;
    if (ttl_seconds > 0) {
        expiry_time = getCurrentTimeMs() + (ttl_seconds * 1000);
    }

    Value v;
    v.str_value = value;
    v.int_value = 0;
    v.expiry_time = expiry_time;
    v.type = 0; // string

    data[key] = v;

    if (ttl_seconds > 0) {
        std::lock_guard<std::mutex> exp_lock(expiry_mutex);
        expiry_queue.push({key, expiry_time});
    }

    return true;
}

std::string KVStore::get(const std::string& key) {
    removeExpired(key);
    std::lock_guard<std::mutex> lock(data_mutex);
    auto it = data.find(key);
    if (it == data.end() || (it->second.expiry_time != -1 && it->second.expiry_time < getCurrentTimeMs())) {
        return "(nil)";
    }
    return it->second.str_value;
}

bool KVStore::del(const std::string& key) {
    std::lock_guard<std::mutex> lock(data_mutex);
    if (data.find(key) != data.end()) {
        data.erase(key);
        return true;
    }
    return false;
}

std::vector<std::string> KVStore::keys(const std::string& pattern) {
    std::vector<std::string> result;
    std::lock_guard<std::mutex> lock(data_mutex);
    
    for (const auto& pair : data) {
        // Skip expired keys
        if (pair.second.expiry_time != -1 && pair.second.expiry_time < getCurrentTimeMs()) {
            continue;
        }
        if (patternMatch(pair.first, pattern)) {
            result.push_back(pair.first);
        }
    }
    return result;
}

long long KVStore::ttl(const std::string& key) {
    std::lock_guard<std::mutex> lock(data_mutex);
    auto it = data.find(key);
    
    if (it == data.end()) {
        return -2; // key doesn't exist
    }
    
    if (it->second.expiry_time == -1) {
        return -1; // no expiry
    }
    
    long long remaining = (it->second.expiry_time - getCurrentTimeMs()) / 1000;
    if (remaining < 0) {
        return -2; // expired
    }
    
    return remaining;
}

bool KVStore::incr(const std::string& key, long long& result) {
    std::lock_guard<std::mutex> lock(data_mutex);
    
    auto it = data.find(key);
    if (it == data.end()) {
        // Create new integer value
        Value v;
        v.int_value = 1;
        v.str_value = "1";
        v.expiry_time = -1;
        v.type = 1;
        data[key] = v;
        result = 1;
        return true;
    }
    
    // Check if expired
    if (it->second.expiry_time != -1 && it->second.expiry_time < getCurrentTimeMs()) {
        data.erase(it);
        Value v;
        v.int_value = 1;
        v.str_value = "1";
        v.expiry_time = -1;
        v.type = 1;
        data[key] = v;
        result = 1;
        return true;
    }
    
    if (it->second.type == 1 || it->second.type == 0) {
        try {
            if (it->second.type == 0) {
                it->second.int_value = std::stoll(it->second.str_value);
            }
            it->second.int_value++;
            it->second.str_value = std::to_string(it->second.int_value);
            it->second.type = 1;
            result = it->second.int_value;
            return true;
        } catch (...) {
            return false;
        }
    }
    
    return false;
}

bool KVStore::decr(const std::string& key, long long& result) {
    std::lock_guard<std::mutex> lock(data_mutex);
    
    auto it = data.find(key);
    if (it == data.end()) {
        // Create new integer value
        Value v;
        v.int_value = -1;
        v.str_value = "-1";
        v.expiry_time = -1;
        v.type = 1;
        data[key] = v;
        result = -1;
        return true;
    }
    
    // Check if expired
    if (it->second.expiry_time != -1 && it->second.expiry_time < getCurrentTimeMs()) {
        data.erase(it);
        Value v;
        v.int_value = -1;
        v.str_value = "-1";
        v.expiry_time = -1;
        v.type = 1;
        data[key] = v;
        result = -1;
        return true;
    }
    
    if (it->second.type == 1 || it->second.type == 0) {
        try {
            if (it->second.type == 0) {
                it->second.int_value = std::stoll(it->second.str_value);
            }
            it->second.int_value--;
            it->second.str_value = std::to_string(it->second.int_value);
            it->second.type = 1;
            result = it->second.int_value;
            return true;
        } catch (...) {
            return false;
        }
    }
    
    return false;
}

bool KVStore::lpush(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(data_mutex);
    auto it = data.find(key);
    
    if (it != data.end() && it->second.expiry_time != -1 && it->second.expiry_time < getCurrentTimeMs()) {
        data.erase(it);
        it = data.end();
    }
    
    if (it == data.end()) {
        Value v;
        v.list_value.push_back(value);
        v.type = 2;
        v.expiry_time = -1;
        data[key] = v;
    } else if (it->second.type == 2) {
        it->second.list_value.insert(it->second.list_value.begin(), value);
    } else {
        return false;
    }
    
    return true;
}

bool KVStore::rpush(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(data_mutex);
    auto it = data.find(key);
    
    if (it != data.end() && it->second.expiry_time != -1 && it->second.expiry_time < getCurrentTimeMs()) {
        data.erase(it);
        it = data.end();
    }
    
    if (it == data.end()) {
        Value v;
        v.list_value.push_back(value);
        v.type = 2;
        v.expiry_time = -1;
        data[key] = v;
    } else if (it->second.type == 2) {
        it->second.list_value.push_back(value);
    } else {
        return false;
    }
    
    return true;
}

std::string KVStore::lpop(const std::string& key) {
    std::lock_guard<std::mutex> lock(data_mutex);
    auto it = data.find(key);
    
    if (it == data.end()) {
        return "(nil)";
    }
    
    if (it->second.expiry_time != -1 && it->second.expiry_time < getCurrentTimeMs()) {
        data.erase(it);
        return "(nil)";
    }
    
    if (it->second.type == 2 && !it->second.list_value.empty()) {
        std::string result = it->second.list_value.front();
        it->second.list_value.erase(it->second.list_value.begin());
        if (it->second.list_value.empty()) {
            data.erase(it);
        }
        return result;
    }
    
    return "(nil)";
}

std::string KVStore::rpop(const std::string& key) {
    std::lock_guard<std::mutex> lock(data_mutex);
    auto it = data.find(key);
    
    if (it == data.end()) {
        return "(nil)";
    }
    
    if (it->second.expiry_time != -1 && it->second.expiry_time < getCurrentTimeMs()) {
        data.erase(it);
        return "(nil)";
    }
    
    if (it->second.type == 2 && !it->second.list_value.empty()) {
        std::string result = it->second.list_value.back();
        it->second.list_value.pop_back();
        if (it->second.list_value.empty()) {
            data.erase(it);
        }
        return result;
    }
    
    return "(nil)";
}

long long KVStore::llen(const std::string& key) {
    std::lock_guard<std::mutex> lock(data_mutex);
    auto it = data.find(key);
    
    if (it == data.end()) {
        return 0;
    }
    
    if (it->second.expiry_time != -1 && it->second.expiry_time < getCurrentTimeMs()) {
        data.erase(it);
        return 0;
    }
    
    if (it->second.type == 2) {
        return it->second.list_value.size();
    }
    
    return 0;
}

bool KVStore::subscribe(const std::string& channel, const std::string& client_id) {
    std::lock_guard<std::mutex> lock(data_mutex);
    subscribers[channel].insert(client_id);
    return true;
}

bool KVStore::unsubscribe(const std::string& channel, const std::string& client_id) {
    std::lock_guard<std::mutex> lock(data_mutex);
    auto it = subscribers.find(channel);
    if (it != subscribers.end()) {
        it->second.erase(client_id);
        if (it->second.empty()) {
            subscribers.erase(it);
        }
        return true;
    }
    return false;
}

bool KVStore::publish(const std::string& channel, const std::string& message) {
    std::lock_guard<std::mutex> lock(data_mutex);
    // In a real implementation, we'd send messages to all subscribers
    // For now, just return whether there were subscribers
    return subscribers.find(channel) != subscribers.end();
}

std::set<std::string> KVStore::get_subscribers(const std::string& channel) {
    std::lock_guard<std::mutex> lock(data_mutex);
    auto it = subscribers.find(channel);
    if (it != subscribers.end()) {
        return it->second;
    }
    return {};
}

std::string KVStore::get_stats() {
    std::lock_guard<std::mutex> lock(data_mutex);
    
    size_t total_keys = 0;
    size_t estimated_memory = 0;
    
    for (const auto& pair : data) {
        if (pair.second.expiry_time == -1 || pair.second.expiry_time >= getCurrentTimeMs()) {
            total_keys++;
            estimated_memory += pair.first.size() + pair.second.str_value.size();
            if (pair.second.type == 2) {
                for (const auto& item : pair.second.list_value) {
                    estimated_memory += item.size();
                }
            }
        }
    }
    
    std::stringstream ss;
    ss << "Total keys: " << total_keys << "\n";
    ss << "Expired keys cleaned: " << stats_expired_cleaned << "\n";
    ss << "Estimated memory usage: " << estimated_memory / 1024.0 << " KB\n";
    ss << "Max keys limit: " << max_keys << "\n";
    
    return ss.str();
}

bool KVStore::save(const std::string& filename) {
    std::lock_guard<std::mutex> lock(data_mutex);
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    file << "{\n  \"keys\": [\n";
    
    bool first = true;
    for (const auto& pair : data) {
        // Skip expired keys
        if (pair.second.expiry_time != -1 && pair.second.expiry_time < getCurrentTimeMs()) {
            continue;
        }
        
        if (!first) file << ",\n";
        first = false;
        
        file << "    {\n";
        file << "      \"key\": \"" << pair.first << "\",\n";
        file << "      \"value\": \"" << pair.second.str_value << "\",\n";
        
        long long ttl_val = -1;
        if (pair.second.expiry_time != -1) {
            ttl_val = (pair.second.expiry_time - getCurrentTimeMs()) / 1000;
        }
        file << "      \"ttl\": " << ttl_val << ",\n";
        file << "      \"type\": " << pair.second.type << "\n";
        file << "    }";
    }
    
    file << "\n  ]\n}\n";
    file.close();
    
    return true;
}

bool KVStore::load(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    std::string key, value;
    long long ttl_val = -1;
    
    std::lock_guard<std::mutex> lock(data_mutex);
    data.clear();
    
    while (std::getline(file, line)) {
        if (line.find("\"key\"") != std::string::npos) {
            size_t start = line.find("\"") + 1;
            size_t end = line.find("\"", start);
            key = line.substr(start, end - start);
        }
        if (line.find("\"value\"") != std::string::npos) {
            size_t start = line.find("\"", line.find(":") + 1) + 1;
            size_t end = line.find("\"", start);
            value = line.substr(start, end - start);
        }
        if (line.find("\"ttl\"") != std::string::npos) {
            size_t start = line.find(":") + 1;
            ttl_val = std::stoll(line.substr(start));
        }
        
        if (!key.empty() && line.find("}") != std::string::npos && line.find("]}") == std::string::npos) {
            set(key, value, ttl_val);
            key.clear();
            value.clear();
            ttl_val = -1;
        }
    }
    
    file.close();
    return true;
}

void KVStore::cleanup() {
    std::lock_guard<std::mutex> lock(data_mutex);
    
    std::vector<std::string> to_delete;
    long long now = getCurrentTimeMs();
    
    for (const auto& pair : data) {
        if (pair.second.expiry_time != -1 && pair.second.expiry_time < now) {
            to_delete.push_back(pair.first);
        }
    }
    
    for (const auto& key : to_delete) {
        data.erase(key);
        stats_expired_cleaned++;
    }
}

void KVStore::lruEvict() {
    // Simple LRU: remove first key (oldest)
    if (!data.empty()) {
        data.erase(data.begin());
    }
}
