#pragma once

#include <vector>
#include <cstdint>
#include "stat.h"
#include "fs_context.h"

struct FSContext;

void block_manager_init(FSContext &ctx);

int block_alloc(FSContext &ctx);

void block_free(FSContext &ctx, int block_num);
