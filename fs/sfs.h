#pragma once
#include <string>
#include <vector>
#include <string>
#include "stat.h"
#include "fs_context.h"
#include "inode.h"
#include "dir.h"
#include "block_manager.h"
#include "disk.h"

bool sfs_init(FSContext &ctx, const std::string &disk_image);
int sfs_create(FSContext &ctx, const std::string &path);
int sfs_mkdir(FSContext &ctx, const std::string &path);
int sfs_open(FSContext &ctx, const std::string &path);
int sfs_read(FSContext &ctx, int fd, char* buf, int size);
int sfs_write(FSContext &ctx, int fd, const char* buf, int size);
bool sfs_seek(FSContext &ctx, int fd, int offset, int whence);
std::vector<std::string> sfs_listdir(FSContext &ctx, const std::string &path);
bool sfs_remove(FSContext &ctx, const std::string &path);
