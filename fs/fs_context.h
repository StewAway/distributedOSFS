#pragma once
#include <string>
#include <map>
#include <vector>
#include <memory>
#include "disk.h"
#include "block_cache_controller.h"

struct OpenFile {
    int inum;
    int offset;
};

struct FSContext {
    std::string disk_image;
    std::vector<bool> block_bitmap;
    std::vector<bool> inode_bitmap;
    
    // File descriptor table & allocator
    std::map<int, OpenFile> fd_table;
    int next_fd = 3;

    // Block cache controller for in-memory storage and disk storage
    uint64_t mount_id;
    std::shared_ptr<Disk> disk;
    std::shared_ptr<BlockCacheController> cache_controller; 
    bool use_cache = true; // toggle default

    FSContext(uint64_t id) : mount_id(id) {}

    void init_cache(size_t cache_capacity_blocks, size_t block_size) {
        if (!disk) throw std::runtime_error("Disk must be initialized before cache.");
        cache_controller = std::make_shared<BlockCacheController>(cache_capacity_blocks, block_size, disk);
    }
};
