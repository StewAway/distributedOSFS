#include "block_cache_controller.h"
#include "disk.h" // disk I/O class

BlockCacheController::BlockCacheController(
    size_t capacity_blocks,
    size_t block_size,
    std::shared_ptr<Disk> disk) : 
    capacity_blocks_(capacity_blocks),
    block_size_(block_size),
    disk_(disk),
    cache_(capacity_blocks) {}

const std::vector<char>& BlockCacheController:getBlock(uint64_t mount_id, uint64_t block_num) {
    BlockKey key = {mount_id, block_num};
    if (cache_.contains(key)) {
        return cache_.get(key).data;
    }

    // Cache miss: read from disk
    std::vector<char> buf(block_size_);
    disk_->disk_read(block_num, buf.data());

    BlockEntry entry{std::move(buf), false};
    cache_.put(key, entry);
    evictIfNeeded();

    return cache_.get(key).data;
}

void BlockCacheController::writeBLock(uint64_t mount_id, uint64_t block_num, const char* buf) {
    BlockKey key = {mount_id, block_num};
    if (!cache_.contains(key)) {
        std::vector<char> tmp(block_size_);
        disk_->disk_read(block_num, tmp.data());
        cache_.put(key, BlockEntry{std::move(tmp, false});
    }

    BlockEntry& entry = cache_.get(key);
    std::memcpy(entry.data.data(), buf, BLOCK_SIZE);
    entry.dirty = true;
    evictIfNeeded();
}

void BlockCacheController::flushEntry(const BlockKey& key, const BlockEntry& entry) {
    disk_->disk_write(key.second, entry.data.data());
}

void BlockCacheController::evictIfNeeded() {
    while (cache_.size() > capacity_blocks_) {
        auto evicted = cache_.evictLeastRecentlyUsed();
        if (evicted.has_value()) {
            const auto& [key, entry] = evicted.value();
            if (entry.dirty) {
                flushEntry(key, entry);
            }
        }
    }
}

void BlockCacheController::flushAll() {
    for (const auto& [key, entry] : cache_.getAll()) {
        if (entry.dirty) {
            flushEntry(key, entry);
        }
    }
}
