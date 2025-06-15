#include "disk.h"
#include <fstream>
#include <vector>
#include <iostream>

Disk::~Disk() {
    if (file_.is_open()) return file_.close();
}

bool Disk::disk_init(const std::string& path) {
    // Try open existing image
    file_.open(path, std::ios::in | std::ios::out | std::ios::binary);
    if (!file_.is_open()) {
        // Create file and zero-fill
        std::ofstream create(path, std::ios::out | std::ios::binary);
        if (!create.is_open()) {
            std::cerr<<"[Disk] Failed to create "<<path<<"\n";
            return false;
        }
        std::vector<char> zeros((size_t)BLOCK_SIZE * NUM_BLOCKS, 0);
        create.write(zeros.data(), zeros.size());
        create.close();
        file_.open(DISK_IMAGE, std::ios::in | std::ios::out | std::ios::binary);
    }
    if (!file_is_open()) {
        std::cerr<<"[Disk] Failed to open "<<path<<"\n";
        return false;
    }
    return true;
}

bool Disk::disk_read(int block_num, char* buffer) {
    if (block_num < 0 || block_num >= NUM_BLOCKS) return false;
    file_.seekg(block_num * BLOCK_SIZE);
    file_.read(buffer, BLOCK_SIZE);
    return file_.good();
}

bool Disk::disk_write(int block_num, const char* buffer) {
    if (block_num < 0 || block_num >= NUM_BLOCKS) return false;
    file_.seekg(block_num * BLOCK_SIZE);
    file_.write(buffer, BLOCK_SIZE);
    return file_.good();
}

int Disk::disk_get_block_size() { 
    return BLOCK_SIZE;
}

int Disk::disk_get_num_blocks() {
    return NUM_BLOCKS;
}
