#pragma once

#include <vector>
#include <memory>
#include <cstdint>
#include <utility>
#include <unordered_map>
#include <optional>
#include "lru_cache.h"
#include <mutex>

class Disk; // foward declare for disk I/O

class BlockCacheController {
public:
    using BlockKey = uint64_t; // block_num

    struct BlockEntry {
        std::vector<char> data;
        bool dirty;
    };

    BlockCacheController(size_t capacity_blocks, size_t block_size, std::shared_ptr<Disk> disk);

    const std::vector<char>& getBlock(uint64_t block_num);

    void writeBlock(uint64_t block_num, const char* buf);

    void flushAll();

private:
    size_t capacity_blocks_;
    size_t block_size_;
    std::shared_ptr<Disk> disk_;
    std::mutex mutex_;
    // switch to LFU if needed
    LRUCache<BlockKey, BlockEntry> cache_;

    void evictIfNeeded();
    void flushEntry(const BlockKey& key, const BlockEntry& entry);
};
