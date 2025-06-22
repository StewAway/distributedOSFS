#pragma once
#include <unordered_map>
#include <iostream>
#include <string>
#include <optional>

using namespace std;

struct LFUNode {
    std::string key;
    std::string value;
    int freq;
    LFUNode* prev;
    LFUNode* next;

    LFUNode(const std::string& k, const std::string& v) : key(k), value(v), freq(1), prev(nullptr), next(nullptr) {}
};

class LFULinkedList {
public:
    LFUNode* head;
    LFUNode* tail;
    int size;

    LFULinkedList() {
        head = new LFUNode("", "");
        tail = new LFUNode("", "");
        head->next = tail;
        tail->prev = head;
        size = 0;
    }

    void remove(LFUNode* node) {
        if (!node || size == 0) return;
        node->prev->next = node->next;
        node->next->prev = node->prev;
        --size;
    }

    void evict() {
        if (size == 0) return;
        remove(tail->prev);
    }

    void add(LFUNode* node) {
        if (!node) return;
        node->next = head->next;
        head->next->prev = node;
        head->next = node;
        node->prev = head;
        ++size;
    }

    LFUNode* get_tail() {
        return size == 0 ? nullptr : tail->prev;
    }
};

class LFUCache {
private:
    unordered_map<std::string, LFUNode*> map_key;
    unordered_map<int, LFULinkedList*> map_freq;
    int capacity;
    int size;
    int min_freq;
    size_t total_gets = 0;
    size_t hit_count = 0;
    size_t miss_count = 0;

public:
    explicit LFUCache(int capacity) : capacity(capacity), size(0), min_freq(1) {}

    std::optional<std::string> get(const std::string& key) {
        ++total_gets;
        if (map_key.find(key) != map_key.end()) {
            ++hit_count;
            LFUNode* node = map_key[key];
            map_freq[node->freq]->remove(node);
            if (min_freq == node->freq && map_freq[node->freq]->size == 0) ++min_freq;
            ++node->freq;
            if (map_freq.find(node->freq) == map_freq.end()) map_freq[node->freq] = new LFULinkedList();
            map_freq[node->freq]->add(node);
            return node->value;
        }
        ++miss_count;
        return std::nullopt;
    }

    void put(const std::string& key, const std::string& value) {
        if (capacity == 0) return;
        if (map_key.find(key) != map_key.end()) {
            LFUNode* node = map_key[key];
            map_freq[node->freq]->remove(node);
            node->value = value;
            ++node->freq;
            if (min_freq == node->freq - 1 && map_freq[min_freq]->size == 0) ++min_freq;
            if (map_freq.find(node->freq) == map_freq.end()) map_freq[node->freq] = new LFULinkedList();
            map_freq[node->freq]->add(node);
        } else {
            if (size == capacity) {
                LFUNode* last = map_freq[min_freq]->get_tail();
                if (last) {
                    map_key.erase(last->key);
                    map_freq[min_freq]->evict();
                    --size;
                }
            }
            LFUNode* node = new LFUNode(key, value);
            if (map_freq.find(1) == map_freq.end()) map_freq[1] = new LFULinkedList();
            map_freq[1]->add(node);
            map_key[key] = node;
            ++size;
            min_freq = 1;
        }
    }

    void remove(const std::string& key) {
        if (map_key.find(key) == map_key.end()) return;
        LFUNode* node = map_key[key];
        map_freq[node->freq]->remove(node);
        map_key.erase(key);
        --size;
    }

    void print_stats() {
        std::cout << "Total GETs: " << total_gets << "\n";
        std::cout << "Cache Hits: " << hit_count << "\n";
        std::cout << "Cache Misses: " << miss_count << "\n";
        if (total_gets > 0) {
            std::cout << "Hit Rate: " << (100.0 * hit_count / total_gets) << "%\n";
        }
    }
};
