#pragma once

#include <cstdint>

// === File System Constants ===

constexpr int BLOCK_SIZE = 4096;         // bytes per block
constexpr int NDIRECT = 12;              // direct pointers per inode
constexpr int NINDIRECT = BLOCK_SIZE / sizeof(uint32_t); // entries in an indirect block
constexpr int NUM_INODES = 128;
constexpr int NUM_BLOCKS = 10240;         // total number of blocks in disk
constexpr int RESERVED_BLOCKS = 1 + 16; // 1 SUPERBLOCK + 16 INODE BLOCKS

// Cache Stats
constexpr int CACHE_NUM_BLOCKS = 1024;

// Optional: max file size calculation
constexpr int MAX_FILE_BLOCKS = NDIRECT + NINDIRECT;
constexpr int MAX_FILE_SIZE = MAX_FILE_BLOCKS * BLOCK_SIZE;

