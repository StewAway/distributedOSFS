#include "block_manager.h"
#include "stat.h"


std::vector<bool> block_bitmap;

void block_manager_init() {
    block_bitmap.assign(NUM_BLOCKS, false);

    for (int i = 0;i < RESERVED_BLOCKS; ++i) {
        block_bitmap[i] = true;
    }
}

int block_alloc() {
    for (int i = RESERVED_BLOCKS;i < NUM_BLOCKS; ++i) {
        if (!block_bitmap[i]) {
            block_bitmap[i] = true;
            return i;
        }
    }
    return -1;
}

void block_free(int block_num) {
    if (block_num >= RESERVED_BLOCKS && block_num < NUM_BLOCKS) {
        block_bitmap[block_num] = false;
    }
}
