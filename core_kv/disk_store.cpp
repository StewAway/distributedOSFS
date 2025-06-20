// disk_store.cpp
#include "disk_store.h"
#include <rocksdb/db.h>
#include <iostream>

DiskStore::DiskStore(const std::string& db_path) {
    rocksdb::Options options;
    options.create_if_missing = true;
    rocksdb::Status status = rocksdb::DB::Open(options, db_path, &db_);
    if (!status.ok()) {
        std::cerr << "[DiskStore] Failed to open RocksDB: " << status.ToString() << std::endl;
        db_ = nullptr;
    }
}

DiskStore::~DiskStore() {
    delete db_;
}

bool DiskStore::put(const std::string& key, const std::string& value) {
    if (!db_) return false;
    rocksdb::Status status = db_->Put(rocksdb::WriteOptions(), key, value);
    return status.ok();
}

bool DiskStore::get(const std::string& key, std::string& value_out) {
    if (!db_) return false;
    rocksdb::Status status = db_->Get(rocksdb::ReadOptions(), key, &value_out);
    return status.ok();
}

bool DiskStore::remove(const std::string& key) {
    if (!db_) return false;
    rocksdb::Status status = db_->Delete(rocksdb::WriteOptions(), key);
    return status.ok();
}
