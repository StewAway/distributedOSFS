#include "dir.h"
#include "inode.h"
#include "disk.h"
#include <cstring>

struct DirEntry {
    int inum;
    char name[252];
};

int dir_lookup(int dir_inum, const std::string &name) {
    Inode dir_inode;

    if (!inode_read(dir_inum, dir_inode)) return -1;
    
    char block[BLOCK_SIZE];
    // Search in direct blocks
    for (int i = 0; i < NDIRECT; ++i) {
        if (dir_inode.direct[i] == 0) continue;
        if (!disk_read(dir_inode.direct[i], block)) return false;

        DirEntry* entries = (DirEntry*) block;
        int num_entries = BLOCK_SIZE / sizeof(DirEntry);

        for (int j = 0; j < num_entries; ++j) {
            if (entries[j].inum != 0 && name == entries[j].name) {
                int inum_out = entries[j].inum;
                return inum_out;
            }
        }
    }

    // Search in indirect blocks
    if (dir_inode.indirect != 0) {
        uint32_t indirect_block[NINDIRECT];
        if (!disk_read(dir_inode.indirect, (char*)indirect_block)) return false;

        for (int k = 0; k < NINDIRECT; ++k) {
            if (indirect_block[k] == 0) continue;
            if (!disk_read(indirect_block[k], block)) return false;

            DirEntry* entries = (DirEntry*) block;
            int num_entries = BLOCK_SIZE / sizeof(DirEntry);

            for (int j = 0; j < num_entries; ++j) {
                if (entries[j].inum != 0 && name == entries[j].name) {
                    int inum_out = entries[j].inum;
                    return inum_out;
                }
            }
        }
    }

    return -1;
}

int dir_add(int dir_inum, const std::string &name, int inum) {
    Inode dir_inode;
    if (!inode_read(dir_inum, dir_inode)) return false;

    char block[4096];

    // Search for empty spot in direct blocks
    for (int i = 0; i < NDIRECT; ++i) {
        if (dir_inode.direct[i] == 0) {
            dir_inode.direct[i] = block_alloc();
            inode_write(dir_inum, dir_inode);
            std::memset(block, 0, sizeof(block));
        } else {
            if (!disk_read(dir_inode.direct[i], block)) return false;
        }

        DirEntry* entries = (DirEntry*) block;
        int num_entries = BLOCK_SIZE / sizeof(DirEntry);

        for (int j = 0; j < num_entries; ++j) {
            if (entries[j].inum == 0) {
                entries[j].inum = inum;
                std::strncpy(entries[j].name, name.c_str(), sizeof(entries[j].name) - 1);
                entries[j].name[sizeof(entries[j].name) - 1] = '\0';
                return disk_write(dir_inode.direct[i], block);
            }
        }
    }

    // Search for empty spot in indirect blocks
    if (dir_inode.indirect == 0) {
        dir_inode.indirect = block_alloc();
        inode_write(dir_inum, dir_inode);
        char zero[BLOCK_SIZE] = {0};
        disk_write(dir_inode.indirect, zero);
    }

    uint32_t indirect_block[NINDIRECT];
    if (!disk_read(dir_inode.indirect, (char*)indirect_block)) return false;

    for (int k = 0; k < NINDIRECT; ++k) {
        if (indirect_block[k] == 0) {
            indirect_block[k] = block_alloc();
            char zero[BLOCK_SIZE] = {0};
            disk_write(indirect_block[k], zero);
            disk_write(dir_inode.indirect, (char*)indirect_block);
        }

        if (!disk_read(indirect_block[k], block)) return false;
        DirEntry* entries = (DirEntry*) block;
        int num_entries = BLOCK_SIZE / sizeof(DirEntry);

        for (int j = 0; j < num_entries; ++j) {
            if (entries[j].inum == 0) {
                entries[j].inum = inum;
                std::strncpy(entries[j].name, name.c_str(), sizeof(entries[j].name) - 1);
                entries[j].name[sizeof(entries[j].name) - 1] = '\0';
                return disk_write(indirect_block[k], block);
            }
        }
    }

    return false;
}

int dir_list(int dir_inum, std::vector<std::string> &out) {
    Inode dir_inode;
    if (!inode_read(dir_inum, dir_inode)) return false;

    char block[BLOCK_SIZE];

    // Search in direct blocks
    for (int i = 0; i < NDIRECT; ++i) {
        if (dir_inode.direct[i] == 0) continue;
        if (!disk_read(dir_inode.direct[i], block)) return false;

        DirEntry* entries = (DirEntry*) block;
        int num_entries = 4096 / sizeof(DirEntry);

        for (int j = 0; j < num_entries; ++j) {
            if (entries[j].inum != 0 && name == entries[j].name) {
                inum_out = entries[j].inum;
                return true;
            }
        }
    }

    // Search in indirect blocks
    if (dir_inode.indirect != 0) {
        uint32_t indirect_block[NINDIRECT];
        if (!disk_read(dir_inode.indirect, (char*)indirect_block)) return false;

        for (int k = 0; k < NINDIRECT; ++k) {
            if (indirect_block[k] == 0) continue;
            if (!disk_read(indirect_block[k], block)) return false;

            DirEntry* entries = (DirEntry*) block;
            int num_entries = BLOCK_SIZE / sizeof(DirEntry);

            for (int j = 0; j < num_entries; ++j) {
                if (entries[j].inum != 0 && name == entries[j].name) {
                    inum_out = entries[j].inum;
                    return true;
                }
            }
        }
    }

    return false;
}

int dir_remove(int dir_inum, const std::string &name) {
    Inode dir_inode;
    if (!inode_read(dir_inum, dir_inode)) return false;
    
    // Traverse 12 direct pointers
    char block[BLOCK_SIZE];
    for (int i = 0;i < NDIRECT; ++i) {
        if (dir_inode.direct[i] == 0) continue;
        if (!disk_read(dir_inode.direct[i], block)) return -1;
        
        DirEntry* entries = (DirEntry*) block;
        int num_entries = BLOCK_SIZE / sizeof(DirEntry);

        for (int j = 0;j < num_entries; ++j) {
            if (entries[j].inum != 0 && strcmp(name, entries[j].name) == 0) {
                entries[j].inum = 0;
                std::memset(entries[j].name, 0, sizeof(entries[j].name));
                return disk_write(dir_inode.direct[i], block);
            }
        }
    }

    // Traverse indirect block if theres any
    if (dir_inode.indirect != 0) {
        uint32_t indirect_block[NINDIRECT];
        if (!disk_read(dir_inode.indirect, (char*)indirect_block)) return false;

        for (int k = 0;k < NINDIRECT; ++k) {
            if (indirect_block[k] == 0) continue;
            if (!disk_read(indirect_block[k], block)) return false;

            DirEntry* entries = (DirEntry*) block;
            int num_entries = BLOCK_SIZE / sizeof(DirEntry);

            for (int j = 0;j < num_entries; ++j) {
                if (entries[j].inum != 0 && name == entries[j].name) {
                    entries[j].inum = 0;
                    std::memset(entries[j].name, 0, sizeof(entries[j].name));
                    return disk_write(indirect_block[k], block);
                }
            }
        }
    }
    return false;
}

