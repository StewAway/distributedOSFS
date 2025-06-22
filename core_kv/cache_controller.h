#pragma once
#include <string>
#include <optional>
#include <iostream>
#include "lru_cache.h"
#include "lfu_cache.h"

enum class CachePolicy {
    LRU,
    LFU
};

class CacheController {
    int hits_ = 0, misses_ = 0;
public:
    CacheController(int capacity, CachePolicy policy = CachePolicy::LRU)
        : policy_(policy), lru_(capacity), lfu_(capacity) {}

    std::optional<std::string> get(const std::string& key) {
        if (policy_ == CachePolicy::LRU) {
            auto val = lru_.get(key);
            if (val) {
                hits_++;
                std::cout<<"[CacheHit] LRU policy "<<key<<"\n";
            } else {
                misses_++;
                std::cout<<"[CacheMiss] LFU policy "<<key<<"\n";
            }
            return val;
        }
        else {
            auto val = lfu_.get(key);
            if (val) {
                hits_++;
                std::cout<<"[CacheHit] LFU policy "<<key<<"\n";
            } else {
                misses_++;
                std::cout<<"[CacheMiss] LFU policy "<<key<<"\n";
            }
            return val;
        }
    }

    void put(const std::string& key, const std::string& value) {
        if (policy_ == CachePolicy::LRU) lru_.put(key, value);
        else lfu_.put(key, value);
    }

    void remove(const std::string& key) {
        if (policy_ == CachePolicy::LRU) lru_.remove(key);
        else lfu_.remove(key);
    }

    void print_stats() const {
        std::cout<<"--------------Cache Stats---------------\n";
        std::cout << "[CacheController] Using ";
        if (policy_ == CachePolicy::LRU) {
            std::cout << "LRU:\n";
        } else {
            std::cout << "LFU:\n";
        }
        int total_gets = hits_ + misses_;
        std::cout<<"Total GETs: "<<total_gets<<"\n";
        std::cout<<"Cache Hits: "<<hits_<<"\n";
        std::cout<<"Cache Misses: "<<misses_<<"\n";
        if (total_gets != 0) {
            double hit_rate = 100.0 * hits_ / total_gets;
            std::cout<<"Hit Rate: "<<hit_rate<<"%\n";
        }
    }

private:
    CachePolicy policy_;
    LRUCache lru_;
    LFUCache lfu_;
};
