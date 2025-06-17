#pragma once
#include <cstdint>
#include "stat.h"
#include "fs_context.h"
#include "disk.h"

struct FSContext;

struct Inode {
    uint32_t mode; // file type and permissions
    uint32_t size; // file size in bytes
    uint32_t direct[NDIRECT]; //direct block pointers
    uint32_t indirect; // single-indirect block pointer
};

void inode_init(FSContext &ctx);
bool inode_read(FSContext &ctx, int inum, Inode &out);
bool inode_write(FSContext &ctx, int inum, const Inode &in);
int inode_alloc(FSContext &ctx);
