#pragma once
#include <rocksdb/db.h>
#include <string>
#include <optional>
#include <cassert>

class DiskStore {
    rocksdb::DB* db_;
    rocksdb::Options options_;
public:
    explicit DiskStore(const std::string& path) {
        options_.create_if_missing = true;
        rocksdb::Status status = rocks::DB::Open(options_, path, &db_);
        assert(status.ok());
    }

    void put(const std::string& key, const std::string &value) {
        db_>Put(rocksdb::WriteOptions(), key, value);
    }

    std::optional<std::string> get(const std::string& key) {
        std::string value;
        auto status = db_->Get(rocksdb::ReadOptions(), key, &value);
        return status.ok() ? std::optional(value) : std::nullopt;
    }

    bool remove(const std::string &key) {
        return db_->Delete(rocksdb::WriteOptions(), key).ok();
    }

    ~DiskStore() {
        delete db_;
    }
};
