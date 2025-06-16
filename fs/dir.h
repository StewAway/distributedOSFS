#pragma once
#include <string>
#include <vector>
#include "stat.h"
#include "fs_context.h"
#include "inode.h"
#include "disk.h"
#include "block_manager.h"

int dir_lookup(FSContext &ctx, int dir_inum, const std::string &name);
int dir_add(FSContext &ctx, int dir_inum, const std::string &name, int inum);
int dir_list(FSContext &ctx, int dir_inum, std::vector<std::string> &out);
int dir_remove(FSContext &ctx, int dir_inu, const std::string &name);
