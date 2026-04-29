#ifndef KVSTORE_H
#define KVSTORE_H

#include <string>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <chrono>
#include <memory>
#include <vector>
#include <set>

struct ExpiryEntry {
    std::string key;
    long long expiry_time; // milliseconds since epoch

    bool operator>(const ExpiryEntry& other) const {
        return expiry_time > other.expiry_time;
    }
};

class KVStore {
public:
    KVStore(size_t max_keys = 10000);
    ~KVStore();

    // Basic operations
    bool set(const std::string& key, const std::string& value, long long ttl_seconds = -1);
    std::string get(const std::string& key);
    bool del(const std::string& key);
    std::vector<std::string> keys(const std::string& pattern);
    long long ttl(const std::string& key); // -1: no expiry, -2: key doesn't exist

    // Bonus: Integer operations
    bool incr(const std::string& key, long long& result);
    bool decr(const std::string& key, long long& result);

    // Bonus: List operations
    bool lpush(const std::string& key, const std::string& value);
    bool rpush(const std::string& key, const std::string& value);
    std::string lpop(const std::string& key);
    std::string rpop(const std::string& key);
    long long llen(const std::string& key);

    // Bonus: Pub/Sub
    bool subscribe(const std::string& channel, const std::string& client_id);
    bool unsubscribe(const std::string& channel, const std::string& client_id);
    bool publish(const std::string& channel, const std::string& message);
    std::set<std::string> get_subscribers(const std::string& channel);

    // Statistics and persistence
    std::string get_stats();
    bool save(const std::string& filename);
    bool load(const std::string& filename);

    // Cleanup
    void cleanup();

private:
    struct Value {
        std::string str_value;
        std::vector<std::string> list_value;
        long long int_value;
        long long expiry_time; // -1 = no expiry, 0 = expired
        int type; // 0: string, 1: integer, 2: list
    };

    std::unordered_map<std::string, Value> data;
    std::priority_queue<ExpiryEntry, std::vector<ExpiryEntry>, std::greater<ExpiryEntry>> expiry_queue;
    std::unordered_map<std::string, std::set<std::string>> subscribers; // channel -> client_ids
    
    mutable std::mutex data_mutex;
    mutable std::mutex expiry_mutex;
    size_t max_keys;
    long long stats_expired_cleaned = 0;

    void removeExpired(const std::string& key);
    bool patternMatch(const std::string& str, const std::string& pattern);
    long long getCurrentTimeMs();
    void lruEvict();
};

#endif // KVSTORE_H
