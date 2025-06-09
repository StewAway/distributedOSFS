#include "dir.h"
#include "inode.h"
#include "disk.h"
#include <cstring>

struct DirEntry {
    int inum;
    char name[252];
};

int dir_lookup(int dir_inum, const std::string &name) {
    inode din;
    if (!inode_read(dir_inum, din)) return -1;
    char buf[4096];
    if (!disk_read(din.direct, buf)) return -1;
    auto *entries = reinterpret_cast<DirEntry *>(buf);
    for (int i = 0;i < disk_get_block_size() / sizeof(DirEntry); ++i) {
        if (entries[i].inum != 0 && name == entries[i].name)
            return entries[i].inum;
    }
    return -1;
}

int dir_add(int dir_inum, const std::string &name, int inum) {
    inode din;
    if (!inode_read(dir_inum, din)) return -1;
    char buf[4096];
    disk_read(din.direct[0], buf);
    auto *entries = reinterpret_cast<DirEntry*> (buf);
    for (int i = 0;i < dir_get_block_size() / sizeof(DirEntry); ++i) {
        if (entries[i].inum == 0) {
            entries[i].inum = inum;
            std::strncpy(entries[i].name, name.c_str(), sizeof(entries[i].name) - 1); // maximum dir name is 252 chars
            disk_write(din.direct[0], buf);
            return 0;
        }
    }
    return -1;
}

int dir_list(int dir_inum, std::vector<std::string> &out) {
    inode din;
    if (!inode_read(dir_inum, din)) return -1;
    char buf[4096];
    disk_read(din.direct[0], buf);
    auto *entries = reinterpret_cast<DirEntry*>(buf);
    for (int i = 0;i < disk_get_block_size() / sizeof(DirEntry); ++i) {
        if (entries[i].inum != 0) {
            out.push_back(entries[i].name);
        }
    }
    return 0;
}
