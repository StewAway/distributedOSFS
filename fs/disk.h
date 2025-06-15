#pragma once

#include <string>
#include "stat.h"
#include <fstream>

class Disk {
private:
    std::fstream file_;
public:
    bool disk_init(const std::string& path);
    bool disk_read(int block_num, char* buffer);
    bool disk_write(int block_num, const char* buffer);
    int disk_get_block_size() const { return BLOCK_SIZE; }
    int disk_get_num_blocks() const { return NUM_BLOCKS; }
    ~Disk();
};
