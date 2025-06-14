#include "disk.h"
#include <fstream>
#include <vector>

constexpr const char* DISK_IMAGE = "disk.img";

static std::fstream disk_file;

bool disk_init() {
    disk_file.open(DISK_IMAGE, std::ios::in | std::ios::out | std::ios::binary);
    if (!disk_file.is_open()) {
        std::ofstream create(DISK_IMAGE, std::ios::out | std::ios::binary);
        std::vector<char> zeros(BLOCK_SIZE * NUM_BLOCKS, 0);
        create.write(zeros.data(), zeros.size());
        create.close();
        disk_file.open(DISK_IMAGE, std::ios::in | std::ios::out | std::ios::binary);
    }
    return disk_file.is_open();
}

bool disk_read(int block_num, char* buffer) {
    if (block_num < 0 || block_num >= NUM_BLOCKS) return false;
    disk_file.seekg(block_num * BLOCK_SIZE);
    disk_file.read(buffer, BLOCK_SIZE);
    return disk_file.good();
}

bool disk_write(int block_num, const char* buffer) {
    if (block_num < 0 || block_num >= NUM_BLOCKS) return false;
    disk_file.seekg(block_num * BLOCK_SIZE);
    disk_file.write(buffer, BLOCK_SIZE);
    return disk_file.good();
}

int disk_get_block_size() { 
    return BLOCK_SIZE;
}

int disk_get_num_blocks() {
    return NUM_BLOCKS;
}
