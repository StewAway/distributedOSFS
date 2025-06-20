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
    
    void remove(const std::string& key) {
        cache_.remove(key);
    }

    void print_stats() const {
        std::cout << "Total GETs: " << hits_ + misses_ << "\n";
        std::cout << "Cache Hits: " << hits_ << "\n";
        std::cout << "Cache Misses: " << misses_ << "\n";
        if (hits_ + misses_ != 0) {
            double hit_rate = 100.0 * hits_ / (hits_ + misses_);
            std::cout << "Hit Rate: " << hit_rate << "%\n";
        }
    }
};
