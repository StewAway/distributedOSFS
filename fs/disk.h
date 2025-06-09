#pragma once

#include <string>

bool disk_init();
bool disk_read(int block_num, char* buffer);
bool disk_write(int block_nu, const char* buffer);
int disk_get_block_size();
int disk_get_num_blocks();
