#pragma once
#include <string>
#include <map>
#include <vector>
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
};
