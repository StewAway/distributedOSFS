#include "sfs.h"
#include "disk.h"
#include "inode.h"
#include "dir.h"
#include "block_manager.h"
#include <map>
#include <sstream>
#include <cstring>
#include <iostream>

struct OpenFile {
    int inum;
    int offset;
};

static std::map<int, OpenFile> fd_table;
static int next_fd = 3;

static int lookup_path(const std::string &path) {
    std::stringstream ss(path);
    std::string comp;

    int cur = 1;
    while (std::getline(ss, comp, '/')) {
        if (comp.empty()) continue;
        cur = dir_lookup(cur, comp);
        if (cur < 0) return -1;
    }
    return cur;
}

static int get_data_block_index(Inode &ino, int file_block_index, bool allocate = false) {
    if (file_block_index < NDIRECT) {
        if (allocate && ino.direct[file_block_index] == 0)
            ino.direct[file_block_index] = block_alloc();
        return ino.direct[file_block_index];
    } else {
        if (ino.indirect == 0 && allocate) {
            ino.indirect = block_alloc();
            char zero[BLOCK_SIZE] = {0};
            disk_write(ino.indirect, zero);
        }
        if (ino.indirect == 0) return 0;

        uint32_t indirect_block[NINDIRECT];
        disk_read(ino.indirect, (char*)indirect_block);

        int idx = file_block_index - NDIRECT;
        if (allocate && indirect_block[idx] == 0) {
            indirect_block[idx] = block_alloc();
            disk_write(ino.indirect, (char*)indirect_block);
        }
        return indirect_block[idx];
    }
}

void sfs_init() {
    disk_init(); // Init disk image for allocating data 
    inode_init(); // inode bitmap init for allocating bitmap
    block_manager_init(); // data block bitmap init for allocating data blocks
}

int sfs_create(const std::string &path) {
    auto pos = path.find_last_of('/');
    std::string dir = (pos == std::string::npos ? "/" : path.substr(0, pos));
    std::string name = path.substr(pos + 1);
    
    int parent = lookup_path(dir);
    if (parent < 0) return -1;
    int inum = inode_alloc();
    if (inum < 0) return -1;
    int bno = block_alloc();
    if (bno < 0) return -1;
    
    Inode ino{};
    ino.mode = 0100644;
    ino.size = 0;
    ino.direct[0] = bno;
    ino.indirect = 0;
    inode_write(inum, ino);
    dir_add(parent, name, inum);
    return inum;
}

int sfs_mkdir(const std::string &path) {
    auto pos = path.find_last_of('/');
    std::string dir = (pos == std::string::npos ? "/" : path.substr(0, pos));
    std::string name = path.substr(pos + 1);
    
    int parent = lookup_path(dir);
    if (parent < 0) return -1;
    int inum = inode_alloc();
    if (inum < 0) return -1;
    int bno = block_alloc();
    if (bno < 0) return -1;

    Inode ino{};
    ino.mode = 0040755;
    ino.size = 0;
    ino.direct[0] = bno;
    inode_write(inum, ino);
    char blockBuf[4096] = {0};
    disk_write(bno, blockBuf);
    dir_add(parent, name, inum);
    return inum;
}

int sfs_open(const std::string &path) {
    int inum = lookup_path(path);
    if (inum < 0) inum = sfs_create(path);
    if (inum < 0) return -1;
    int fd = next_fd++;
    fd_table[fd] = {inum, 0};
    return fd;
}

int sfs_read(int fd, char* buf, int size) {
    auto it = fd_table.find(fd);
    if (it == fd_table.end()) return -1;
    // 1. Getting fd for its inum and offset
    OpenFile &of = it->second;
    Inode ino{};
    
    if (!inode_read(of.inum, ino)) return -1;
    int total = 0;
    // Reading bytes until 1) size bytes  and 2) no bytes left to read
    while (total < size && of.offset < ino.size) {
        int block_idx = of.offset / BLOCK_SIZE;
        int inner_offset = of.offset / BLOCK_SIZE;
        int block_no = get_data_block_index(ino, block_idx, true);
        if (block_no == 0) break;

        char blockBuf[BLOCK_SIZE];
        if (!disk_read(block_no, blockBuf)) return -1;

        int chunk = std::min(size - total, BLOCK_SIZE - inner_offset);
        chunk = std::min(chunk, (int)ino.size - of.offset);
        std::memcpy(buf + total, blockBuf + inner_offset, chunk);
        of.offset += chunk;
        total += chunk;
    }
    return total;
}

int sfs_write(int fd, const char* buf, int size) {
    auto it = fd_table.find(fd);
    if (it == fd_table.end()) return -1;
    
    OpenFile &of = it->second;
    Inode ino{};
    if (!inode_read(of.inum, ino)) return -1;
    int total = 0;
    while (total < size) {
        int block_idx = of.offset / BLOCK_SIZE;
        int inner_offset = of.offset % BLOCK_SIZE;
        int block_no = get_data_block_index(ino, block_idx, true);
        if (block_no == 0) return -1;

        char blockBuf[BLOCK_SIZE];
        if (!disk_read(block_no, blockBuf)) return -1;

        int chunk = std::min(size - total, BLOCK_SIZE - inner_offset);
        std::memcpy(blockBuf + inner_offset, buf + total, chunk);
        disk_write(block_no, blockBuf);

        of.offset += chunk;
        total += chunk;
    }
    ino.size = std::max((uint32_t)of.offset, ino.size);
    inode_write(of.inum, ino);
    return total;
}

bool sfs_seek(int fd, int offset, int whence) {
    auto it = fd_table.find(fd);
    if (it == fd_table.end()) return false;
    OpenFile &of = it->second;
    int newOffset;
    Inode ino{};
    if (!inode_read(of.inum, ino)) return false;
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

std::vector<std::string> sfs_listdir(const std::string &path) {
    std::vector<std::string> ans;

    int inum = lookup_path(path);
    if (inum < 0) return ans;
    std::cout<<"Printing sfs_listdir of inum "<<inum<<"\n";
    dir_list(inum, ans);
    return ans;
}

bool sfs_remove(const std::string &path) {
    auto pos = path.find_last_of('/');
    std::string dir = (pos == std::string::npos ? "/" : path.substr(0, pos));
    std::string name = path.substr(pos + 1);
    int parent = lookup_path(dir);
    if (parent < 0) return false;
    int inum = dir_lookup(parent, name);
    if (inum < 0) return false;
    
    Inode ino;
    if (!inode_read(inum, ino)) return false;
    for (int i = 0;i < NDIRECT; ++i)
        if (ino.direct[i])
            block_free(ino.direct[i]); // deallocating/free data blocks
    if (ino.indirect) {
        uint32_t indirect_block[NINDIRECT];
        disk_read(ino.indirect, (char*)indirect_block);
        for (int i = 0;i < NINDIRECT; ++i) {
            if (indirect_block[i] == 0) continue;
            block_free(indirect_block[i]);
        }
        block_free(ino.indirect);
    }
    if (dir_remove(parent, name) < 0) return false;
    Inode empty{};
    inode_write(inum, empty);
    return true;
}
