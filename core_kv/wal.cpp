#include "wal.h"
#include <sstream>
#include <fstream>
#include <cstdio>
#include <iostream>

WAL::WAL(const std::string& filename) {
    logfile_.open(filename, std::ios::app); // append mode
}

WAL::~WAL() {
    if (logfile_.is_open()) logfile_.close();
}

void WAL::appendPut(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    logfile_ << "PUT " << key << " " << value << "\n";
    logfile_.flush();
}

void WAL::appendDelete(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    logfile_ << "DELETE " << key << "\n";
    logfile_.flush();
}

std::string WALEntry::serialize() const {
    std::ostringstream oss;
    if (action == WALAction::PUT) {
        oss << "PUT " << key << " " << value;
    } else {
        oss << "DELETE " << key;
    }
    return oss.str();
}

WALEntry WALEntry::deserialize(const std::string& line) {
    std::istringstream iss(line);
    std::string type, key, value;
    iss >> type >> key;
    if (type == "PUT") {
        iss >> value;
        return {WALAction::PUT, key, value};
    } else {
        return {WALAction::DELETE, key, ""};
    }
}

std::vector<WALEntry> WAL::recover() {
    std::vector<WALEntry> entries;
    
    std::ifstream infile("wal.log");
    if (!infile.is_open()) {
        std::cerr<<"[WAL] Failed to open wal.log for recovery\n";
        return entries;
    }

    std::cout<<"Reading wal.log now\n";
    std::string line;
    while (std::getline(infile, line)) {
        if (!line.empty())
            entries.push_back(WALEntry::deserialize(line));
    }
    infile.close();
    return entries;
}
