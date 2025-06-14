#pragma once
#include <string>
#include <vector>
#include "stat.h"

int dir_lookup(int dir_inum, const std::string &name);
int dir_add(int dir_inum, const std::string &name, int inum);
int dir_list(int dir_inum, std::vector<std::string> &out);
int dir_remove(int dir_inu, const std::string &name);
