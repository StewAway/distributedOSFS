#pragma once
#include <rocksdb/db.h>
#include <string>
#include <optional>
#include <cassert>

class DiskStore {
    rocksdb::DB* db_;
    rocksdb::Options options_;
public:
    explicit DiskStore(const std::string& path);
    bool put(const std::string& key, const std::string &value);
    bool get(const std::string& key, std::string& value_out);
    bool remove(const std::string &key);
    ~DiskStore();
};
