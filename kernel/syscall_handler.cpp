#include "syscall_table.h"
#include <iostream>

std::string syscall_open(const std::vector<std::string>& args) {
    std::string filename = args[0];
    // PLACEHOLDER: handle open file
    return "Opened file: " + filename;
}

std::string syscall_read(const std::vector<std::string>& args) {
    std::string fd = args[0];
    return "Read from fd: " + fd;
}

std::string syscall_write(const std::vector<std::string>& args) {
    std::string fd = args[0];
    std::string content = args[1];
    return "Wrote to fd: " + fd + " content: " + content;
}

std::string dispatch_syscall(const std::string& syscall, const std::vector<std::string>& args) {
    if (syscall == "open") return syscall_open(args);
    if (syscall == "read") return syscall_read(args);
    if (syscall == "write") return syscall_write(args);

    return "Unknown syscall";
}
