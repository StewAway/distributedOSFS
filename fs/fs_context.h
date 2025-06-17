#pragma once
#include <string>
#include "block_manager.h"
#include "inode.h"
#include "disk.h"

struct OpenFile {
    int inum;
    int offset;
};

struct FSContext {
    Disk disk; // new per-mount Disk object
    std::string disk_image;
    std::vector<bool> block_bitmap;
    std::vector<bool> inode_bitmap;
    
    // File descriptor table & allocator
    std::map<int, OpenFile> fd_table;
    int next_fd = 3;

    bool init(const std::string& path) {
        // 1) open/create disk image
        disk_image = path;
        if (!disk.disk_init(path)) return false;
        // 2) init inode table
        inode_init(*this);
        // 3) init block-allocation bitmap
        block_manager_init(*this);
        return true;
    }
};
