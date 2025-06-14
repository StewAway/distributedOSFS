#pragma once
#include <string>
#include <vector>
#include <string>
#include "stat.h"
typedef std::vector<std::string> DirList;

void sfs_init();
int sfs_create(const std::string &path);
int sfs_mkdir(const std::string &path);
int sfs_open(const std::string &path);
int sfs_read(int fd, char* buf, int size);
int sfs_write(int fd, const char* buf, int size);
bool sfs_seek(int fd, int offset, int whence);
std::vector<std::string> sfs_listdir(const std::string &path);
bool sfs_remove(const std::string &path);
