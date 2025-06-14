#pragma once
#include <cstdint>
#include "stat.h"

struct Inode {
    uint32_t mode; // file type and permissions
    uint32_t size; // file size in bytes
    uint32_t direct[NDIRECT]; //direct block pointers
    uint32_t indirect; // single-indirect block pointer
};

void inode_init();
bool inode_read(int inum, Inode &out);
bool inode_write(int inum, const Inode &in);
int inode_alloc();
