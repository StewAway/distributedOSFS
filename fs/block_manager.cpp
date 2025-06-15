#include "block_manager.h"
#include "stat.h"


void block_manager_init(FSContext& ctx) {
    ctx.block_bitmap.assign(NUM_BLOCKS, false);
    // reserve superblock + inode-table blocks:
    for (int i = 0;i < RESERVED_BLOCKS; ++i) {
        ctx.block_bitmap[i] = true;
    }
}

int block_alloc(FSContext& ctx) {
    for (int i = RESERVED_BLOCKS;i < NUM_BLOCKS; ++i) {
        if (!ctx.block_bitmap[i]) {
            ctx.block_bitmap[i] = true;
            return i;
        }
    }
    return -1;
}

void block_free(FSContext& ctx, int block_num) {
    if (block_num >= RESERVED_BLOCKS && block_num < NUM_BLOCKS) {
        ctx.block_bitmap[block_num] = false;
    }
}
