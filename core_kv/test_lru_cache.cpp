#include <iostream>
#include "lru_cache.h"

int main() {
    LRUCache cache(3); // capacity = 3

    // Put operations: fills cache and causes evictions
    cache.put("k1", "v1");
    cache.put("k2", "v2");
    cache.put("k3", "v3");
    cache.put("k4", "v4");
    cache.put("k5", "v5");
    cache.put("k6", "v6");

    std::cout << "=== Cache state after puts ===\n";
    cache.print();

    // Get operations: test hits/misses
    for (int i = 1; i <= 6; ++i) {
        std::string key = "k" + std::to_string(i);
        auto val = cache.get(key);
        if (val) {
            std::cout << "[CacheHit] " << key << " = " << *val << "\n";
        } else {
            std::cout << "[CacheMiss] " << key << "\n";
        }
    }

    std::cout << "=== Final cache state ===\n";
    cache.print();

    return 0;
}
