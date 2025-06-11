#pragma once
#include <cstdin>

#define NDIRECT 12
#define NINDIRECT (BLOCK_SIZE / sizeof(uint32_t))

struct inode {
    uint32_t mode; // file type and permissions
    uint32_t size; // file size in bytes
    uint32_t direct[NDIRECT]; //direct block pointers
    uint32_t indirect; // single-indirect block pointer
};

void inode_init():
bool inode_read(int inum, inode &out);
bool inode_write(int inum, const inode &in);
int inode_alloc();
