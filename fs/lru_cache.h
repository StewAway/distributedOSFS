#pragma once
#include <unordered_map>
#include <list>
#include <utility>
#include <optional>

template <typename Key, typename Value>
class LRUCache {
public:
    explicit LRUCache(size_t capacity) : capacity_(capacity) {}

    bool contains(const Key& key) const {
        return map_.find(key) != map_.end();
    }

    Value& get(const Key& key) {
        auto it = map_.find(key);
        if (it == map_.end()) throw std::runtime_error("Key not found");
        list_.splice(list_.begin(), list_, it->second);
        return it->second->second;
    }

    void put(const Key& key, const Value& value) {
        if (contains(key)) {
            auto it = map_[key];
            it->second = value;
            list_.splice(list_.begin(), list_, it);
        } else {
            if (list_.size() == capacity_) {
                evictLeastRecentlyUsed();
            }
            list_.emplace_front(key, value);
            map_[key] = list_.begin();
        }
    }

    std::optional<std::pair<Key, Value>> evictLeastRecentlyUsed() {
        if (list_.empty()) return std::nullopt;
        auto lru = list_.back();
        map_.erase(lru.first);
        list_.pop_back();
        return lru;
    }

    std::unordered_map<Key, Value> getAll() const {
        std::unordered_map<Key, Value> out;
        for (const auto& [k, v] : list_) {
            out[k] = v;
        }
        return out;
    }

    size_t size() const { return list_.size(); }

private:
    size_t capacity_;
    std::list<std::pair<Key, Value>> list_;
    std::unordered_map<Key, typename std::list<std::pair<Key, Value>>::iterator> map_;
};

