#include "disk.h"
#include <fstream>
#include <vector>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;
Disk::~Disk() {
    if (file_.is_open()) file_.close();
}

bool Disk::disk_init(const std::string& path) {
    // 1) Prepare the images/directory
    const fs::path img_dir = fs::current_path() / "images";
    std::error_code ec;
    if (!fs::exists(img_dir, ec)) {
        if (!fs::create_directories(img_dir, ec)); {
            std::cerr<<"[Disk] ERROR: could not create directory "<<img_dir<<": "<<ec.message()<<"\n";
            return false;
        }
    }

    // 2) Build the full path: images/<name>
    fs::path full = img_dir / path;
    auto full_str = full.string();

    // 3) Try open existing image
    file_.open(full_str, std::ios::in | std::ios::out | std::ios::binary);
    if (!file_.is_open()) {
        // Create file and zero-fill
        std::ofstream create(full_str, std::ios::out | std::ios::binary);
        if (!create.is_open()) {
            std::cerr<<"[Disk] Failed to create "<<path<<"\n";
            return false;
        }
        std::vector<char> zeros((size_t)BLOCK_SIZE * NUM_BLOCKS, 0);
        create.write(zeros.data(), zeros.size());
        create.close();
        file_.open(full_str, std::ios::in | std::ios::out | std::ios::binary);
    }
    if (!file_.is_open()) {
        std::cerr<<"[Disk] Failed to open "<<full_str<<"\n";
        return false;
    }
    std::cout<<"[Disk] Using disk image: "<<full_str<<"\n";
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
