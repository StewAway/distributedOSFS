#pragma once
#include "lru_cache.h"
#include <iostream>

class CacheController {
    LRUCache cache_;
    int hits_ = 0, misses_ = 0;
public:
    explicit CacheController(int capacity): cache_(capacity) {}

    std::optional<std::string> get(const std::string& key) {
        auto val = cache_.get(key);
        if (val) {
            hits_++;
            std::cout<<"[CacheHit] "<<key<<"\n";
        } else {
            misses_++;
            std::cout<<"[CacheMiss] "<<key<<"\n";
        }
        return val;
    }
    void put(const std::string& key, const std::string& value) {
        cache_.put(key, value);
    }
    
    void remove(const std::string& key, const std::string& value) {
        cache_.remove(key, value);
    }

    void print_stats() const {
        std::cout<<"[CacheStats] Hits: "<<hits_<<",Misses: "<<misses_<<"\n";
    }
};
