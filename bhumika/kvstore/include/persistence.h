#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include <string>
#include <vector>
#include <memory>
#include "kvstore.h"

class Persistence {
public:
    static bool save(std::shared_ptr<KVStore> store, const std::string& filename);
    static bool load(std::shared_ptr<KVStore> store, const std::string& filename);
};

#endif // PERSISTENCE_H
