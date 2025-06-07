#pragma once
#include <string>
#include <vector>

std::string syscall_open(const std::vector<std::string>& args);
std::string syscall_read(const std::vector<std::string>& args);
std::string syscall_write(const std::vector<std::string>& args);
std::string dispatch_syscall(const std::string& syscall, const std::vector<std::string>& args);
