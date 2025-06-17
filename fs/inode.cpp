#include "stat.h"
#include "fs_context.h"
#include "inode.h"
#include <vector>
#include <cstring>


const int INODES_PER_BLOCK = BLOCK_SIZE / sizeof(Inode);

void inode_init(FSContext &ctx) {
    ctx.inode_bitmap.assign(NUM_INODES, false);
    ctx.inode_bitmap[0] = true;
    Inode root;
    inode_read(ctx, 1, root);
    if (root.mode == 0) { // first run (root uninitialized) => init root
        root.mode = 040755;
        root.size = 0;
        root.direct[0] = 2; // allocate a data block for root directory "/"
        for (int i = 1;i < NDIRECT; ++i) root.direct[i] = 0;
        root.indirect = 0;
        inode_write(ctx, 1, root);
        ctx.inode_bitmap[1] = true;
    }
}

bool inode_read(FSContext &ctx, int inum, Inode &out) {
    if (inum <= 0 || inum >= NUM_INODES) return false;
    int block = 1 + (inum / INODES_PER_BLOCK);
    int off = inum % INODES_PER_BLOCK;
    std::vector<char> buf(BLOCK_SIZE);
    if (!ctx.disk.disk_read(block, buf.data())) return false;
    std::memcpy(&out, buf.data() + off * sizeof(Inode), sizeof(Inode));
    return true;
}

bool inode_write(FSContext &ctx, int inum, const Inode &in) {
    if (inum <= 0 || inum >= NUM_INODES) return false;
    int block = 1 + (inum / INODES_PER_BLOCK);
    int off = inum % INODES_PER_BLOCK;
    std::vector<char> buf(BLOCK_SIZE);
    ctx.disk.disk_read(block, buf.data());
    std::memcpy(buf.data() + off * sizeof(Inode), &in, sizeof(Inode));
    return ctx.disk.disk_write(block, buf.data());
}

int inode_alloc(FSContext &ctx) {
    for (int i = 1;i < NUM_INODES; ++i) {
        if (!ctx.inode_bitmap[i]) {
            ctx.inode_bitmap[i] = true;
            return i;
        }
    }
    return -1;
}
