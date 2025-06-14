#pragma once

#include <vector>
#include <cstdint>
#include "stat.h"

// Global bitmap: true = block in use, false = free
extern std::vector<bool> block_bitmap;

void block_manager_init();

int block_alloc();

void block_free(int block_num);
