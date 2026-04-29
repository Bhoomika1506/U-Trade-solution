#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

#include <string>
#include <vector>
#include <memory>
#include "kvstore.h"

class CommandParser {
public:
    CommandParser(std::shared_ptr<KVStore> store);
    std::string execute(const std::string& command_line, const std::string& client_id = "");

private:
    std::shared_ptr<KVStore> store;
    std::string client_id;

    // Command handlers
    std::string handleSet(const std::vector<std::string>& args);
    std::string handleGet(const std::vector<std::string>& args);
    std::string handleDel(const std::vector<std::string>& args);
    std::string handleKeys(const std::vector<std::string>& args);
    std::string handleTtl(const std::vector<std::string>& args);
    std::string handleStats(const std::vector<std::string>& args);
    std::string handleSave(const std::vector<std::string>& args);
    std::string handleLoad(const std::vector<std::string>& args);
    std::string handleIncr(const std::vector<std::string>& args);
    std::string handleDecr(const std::vector<std::string>& args);
    std::string handleLpush(const std::vector<std::string>& args);
    std::string handleRpush(const std::vector<std::string>& args);
    std::string handleLpop(const std::vector<std::string>& args);
    std::string handleRpop(const std::vector<std::string>& args);
    std::string handleLlen(const std::vector<std::string>& args);
    std::string handlePublish(const std::vector<std::string>& args);
    std::string handleSubscribe(const std::vector<std::string>& args);
    std::string handleUnsubscribe(const std::vector<std::string>& args);
    std::string handleHelp(const std::vector<std::string>& args);

    std::vector<std::string> tokenize(const std::string& input);
    std::string toUpperCase(std::string str);
};

#endif // COMMAND_PARSER_H
