#include <map>
#include <sstream>
#include <cstring>
#include <iostream>
#include <vector>
#include "fs_context.h"
#include "inode.h"
#include "dir.h"
#include "block_manager.h"
#include "disk.h"
#include "block_cache_controller.h"

static int lookup_path(FSContext &ctx, const std::string &path) {
    std::stringstream ss(path);
    std::string comp;

    int cur = 1;
    while (std::getline(ss, comp, '/')) {
        if (comp.empty()) continue;
        cur = dir_lookup(ctx, cur, comp);
        if (cur < 0) return -1;
    }
    return cur;
}

static int get_data_block_index(FSContext &ctx, Inode &ino, int file_block_index, bool allocate = false) {
    if (file_block_index < NDIRECT) {
        if (allocate && ino.direct[file_block_index] == 0)
            ino.direct[file_block_index] = block_alloc(ctx);
        return ino.direct[file_block_index];
    } else {
        if (ino.indirect == 0 && allocate) {
            ino.indirect = block_alloc(ctx);
            char zero[BLOCK_SIZE] = {0};
            //if (!ctx.disk.disk_write(ino.indirect, zero)) return -1;
            if (ctx.use_cache) {
                ctx.cache_controller->writeBlock(ino.indirect, zero);
            } else {
                if (!ctx.disk->disk_write(ino.indirect, zero)) return -1;
            }
        }
        if (ino.indirect == 0) return 0;

        uint32_t indirect_block[NINDIRECT];
        //if (!ctx.disk.disk_read(ino.indirect, (char*)indirect_block)) return -1;
        if (ctx.use_cache) {
            auto& data = ctx.cache_controller->getBlock(ino.indirect);
            std::memcpy((char*)indirect_block, data.data(), BLOCK_SIZE);
        } else {
            if (!ctx.disk->disk_read(ino.indirect, (char*)indirect_block)) return -1; 
        }

        int idx = file_block_index - NDIRECT;
        if (allocate && indirect_block[idx] == 0) {
            indirect_block[idx] = block_alloc(ctx);
            // if (!ctx.disk.disk_write(ino.indirect, (char*)indirect_block)) return -1;
            if (ctx.use_cache) {
                ctx.cache_controller->writeBlock(ino.indirect, (char*)indirect_block);
            } else {
                if (!ctx.disk->disk_write(ino.indirect, (char*)indirect_block)) return -1;
            }
        }
        return indirect_block[idx];
    }
}

bool sfs_init(FSContext &ctx, const std::string &disk_image) {
    if (!ctx.disk->disk_init(disk_image)) {
        std::cerr<<"Failed to initialize disk.\n";
        return false;
    }
    block_manager_init(ctx);
    inode_init(ctx);
    return true;
}

int sfs_create(FSContext &ctx, const std::string &path) {
    auto pos = path.find_last_of('/');
    std::string dir = (pos == std::string::npos ? "/" : path.substr(0, pos));
    std::string name = path.substr(pos + 1);
    
    int parent = lookup_path(ctx, dir);
    if (parent < 0) return -1;
    int inum = inode_alloc(ctx);
    if (inum < 0) return -1;
    int bno = block_alloc(ctx);
    if (bno < 0) return -1;
    
    Inode ino{};
    ino.mode = 0100644;
    ino.size = 0;
    ino.direct[0] = bno;
    ino.indirect = 0;
    inode_write(ctx, inum, ino);
    dir_add(ctx, parent, name, inum);
    return inum;
}

int sfs_mkdir(FSContext &ctx, const std::string &path) {
    auto pos = path.find_last_of('/');
    std::string dir = (pos == std::string::npos ? "/" : path.substr(0, pos));
    std::string name = path.substr(pos + 1);
    
    int parent = lookup_path(ctx, dir);
    if (parent < 0) return -1;
    int inum = inode_alloc(ctx);
    if (inum < 0) return -1;
    int bno = block_alloc(ctx);
    if (bno < 0) return -1;

    Inode ino{};
    ino.mode = 0040755;
    ino.size = 0;
    ino.direct[0] = bno;
    inode_write(ctx, inum, ino);
    char blockBuf[4096] = {0};
    //ctx.disk.disk_write(bno, blockBuf);
    if (ctx.use_cache) {
        ctx.cache_controller->writeBlock(bno, blockBuf);
    } else {
        ctx.disk->disk_write(bno, blockBuf);
    }
    dir_add(ctx, parent, name, inum);
    return inum;
}

int sfs_open(FSContext &ctx, const std::string &path) {
    int inum = lookup_path(ctx, path);
    if (inum < 0) inum = sfs_create(ctx, path);
    if (inum < 0) return -1;
    int fd = ctx.next_fd++;
    ctx.fd_table[fd] = {inum, 0};
    return fd;
}

int sfs_read(FSContext &ctx, int fd, char* buf, int size) {
    auto it = ctx.fd_table.find(fd);
    if (it == ctx.fd_table.end()) return -1;
    // 1. Getting fd for its inum and offset
    OpenFile &of = it->second;
    Inode ino{};
    
    if (!inode_read(ctx, of.inum, ino)) return -1;
    int total = 0;
    // Reading bytes until 1) size bytes  and 2) no bytes left to read
    while (total < size && of.offset < ino.size) {
        int block_idx = of.offset / BLOCK_SIZE;
        int inner_offset = of.offset / BLOCK_SIZE;
        int block_no = get_data_block_index(ctx, ino, block_idx, true);
        if (block_no == 0) break;

        char blockBuf[BLOCK_SIZE];
        //if (!ctx.disk.disk_read(block_no, blockBuf)) return -1;
        if (ctx.use_cache) {
            auto& data = ctx.cache_controller->getBlock(block_no);
            std::memcpy(blockBuf, data.data(), BLOCK_SIZE);
        } else {
            if (!ctx.disk->disk_read(block_no, blockBuf)) return -1;
        }

        int chunk = std::min(size - total, BLOCK_SIZE - inner_offset);
        chunk = std::min(chunk, (int)ino.size - of.offset);
        std::memcpy(buf + total, blockBuf + inner_offset, chunk);
        of.offset += chunk;
        total += chunk;
    }
    return total;
}

int sfs_write(FSContext &ctx, int fd, const char* buf, int size) {
    auto it = ctx.fd_table.find(fd);
    if (it == ctx.fd_table.end()) return -1;
    
    OpenFile &of = it->second;
    Inode ino{};
    if (!inode_read(ctx, of.inum, ino)) return -1;
    int total = 0;
    while (total < size) {
        int block_idx = of.offset / BLOCK_SIZE;
        int inner_offset = of.offset % BLOCK_SIZE;
        int block_no = get_data_block_index(ctx, ino, block_idx, true);
        if (block_no == 0) return -1;

        char blockBuf[BLOCK_SIZE];
        //if (!ctx.disk.disk_read(block_no, blockBuf)) return -1;
        if (ctx.use_cache) {
            auto& data = ctx.cache_controller->getBlock(block_no);
            std::memcpy(blockBuf, data.data(), BLOCK_SIZE);
        } else {
            if (!ctx.disk->disk_read(block_no, blockBuf)) return -1;
        }

        int chunk = std::min(size - total, BLOCK_SIZE - inner_offset);
        std::memcpy(blockBuf + inner_offset, buf + total, chunk);
        //ctx.disk.disk_write(block_no, blockBuf);
        if (ctx.use_cache) {
            ctx.cache_controller->writeBlock(block_no, blockBuf);
        } else {
            ctx.disk->disk_write(block_no, blockBuf);
        }
        of.offset += chunk;
        total += chunk;
    }
    ino.size = std::max((uint32_t)of.offset, ino.size);
    inode_write(ctx, of.inum, ino);
    return total;
}

bool sfs_seek(FSContext &ctx, int fd, int offset, int whence) {
    auto it = ctx.fd_table.find(fd);
    if (it == ctx.fd_table.end()) return false;
    OpenFile &of = it->second;
    int newOffset;
    Inode ino{};
    if (!inode_read(ctx, of.inum, ino)) return false;
    switch (whence) {
        case 0: newOffset = offset; break;
        case 1: newOffset = of.offset + offset; break;
        case 2: newOffset == ino.size + offset; break;
        default: return false;
    }
    int max_file_bytes = (NDIRECT + NINDIRECT) * BLOCK_SIZE;
    if (newOffset < 0 || newOffset >= max_file_bytes) return false;
    of.offset = newOffset;
    return true;
}

std::vector<std::string> sfs_listdir(FSContext &ctx, const std::string &path) {
    std::vector<std::string> ans;

    int inum = lookup_path(ctx, path);
    if (inum < 0) return ans;
    dir_list(ctx, inum, ans);
    return ans;
}

bool sfs_remove(FSContext &ctx, const std::string &path) {
    auto pos = path.find_last_of('/');
    std::string dir = (pos == std::string::npos ? "/" : path.substr(0, pos));
    std::string name = path.substr(pos + 1);
    int parent = lookup_path(ctx, dir);
    if (parent < 0) return false;
    int inum = dir_lookup(ctx, parent, name);
    if (inum < 0) return false;
    
    Inode ino;
    if (!inode_read(ctx, inum, ino)) return false;
    for (int i = 0;i < NDIRECT; ++i)
        if (ino.direct[i])
            block_free(ctx, ino.direct[i]); // deallocating/free data blocks
    if (ino.indirect) {
        uint32_t indirect_block[NINDIRECT];
        //ctx.disk.disk_read(ino.indirect, (char*)indirect_block);
        if (ctx.use_cache) {
            auto& data = ctx.cache_controller->getBlock(ino.indirect);
            std::memcpy((char*)indirect_block, data.data(), BLOCK_SIZE);
        } else {
            ctx.disk->disk_read(ino.indirect, (char*)indirect_block);
        }
        for (int i = 0;i < NINDIRECT; ++i) {
            if (indirect_block[i] == 0) continue;
            block_free(ctx, indirect_block[i]);
        }
        block_free(ctx, ino.indirect);
    }
    if (dir_remove(ctx, parent, name) < 0) return false;
    Inode empty{};
    inode_write(ctx, inum, empty);
    return true;
}
