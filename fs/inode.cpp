#include "inode.h"
#include "disk.h"
#include <vector>
#include <csstring>

constexpr int NUM_INODES = 128;
constexpr int INODES_PER_BLOCK = disk_get_block_size() / sizeof(inode);

static std::vector<bool> inode_bitmap; // bitmap for availability of inodes;

void inode_init() {
    inode_bitmap.assign(NUM_INODES, false);
    inode_bitmap[0] = true;
    inode root;
    inode_read(1, root);
    if (root.mode == 0) { // first run (root uninitialized) => init root
        root.mode = 040755;
        root.size = 0;
        root.direct[0] = 2; // allocate a data block for root directory "/"
        for (int i = 1;i < inode::NDIRECT; ++i) root.direct[i] = 0;
        root.indirect = 0;
        inode_write(1, root);
        inode_bitmap[1] = true;
    }
}

bool inode_read(int inum, inode &out) {
    if (inum <= 0 || inum >= NUM_INODES) return false;
    int block = 1 + (inum / INODE_PER_BLOCK);
    int off = inum % INODES_PER_BLOCK;
    std::vector<char> buf(disk_get_block_size());
    if (!disk_read(block, buf.data()) return false;
    std::memcpy(&out, buf.data() + off * sizeof(inode), sizeof(inode));
    return true;
}

bool inode_write(int inum, const inode &in) {
    if (inum <= 0 || inum >= NUM_INODES) return false;
    int block = 1 + (inum / INODES_PER_BLOCK);
    int off = inum % INODES_PER_BLOCK;
    std::vector<char> buf(disk_get_block_size());
    disk_read(block, buf.data());
    std::memcpy(buf.data() + off * sizeof(inode), &in, sizeof(inode));
    return disk_write(block, buf.data());
}

int inode_alloc() {
    for (int i = 1;i < NUM_INODES; ++i) {
        if (!inodes_bitmap[i]) {
            inode_bitmap[i] = true;
            return i;
        }
    }
    return -1;
}
