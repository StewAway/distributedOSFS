#pragma once
#include <unordered_map>
#include <string>
#include <optional>

struct LinkedList {
    std::string key;
    std::string val;
    LinkedList *prev;
    LinkedList *next;

    LinkedList() : key(""), val(""), prev(nullptr), next(nullptr) {}
    LinkedList(const std::string& k, const std::string& v)
        : key(k), val(v), prev(nullptr), next(nullptr) {}
};

class LRUCache {
private: 
    std::unordered_map<std::string, LinkedList*> hashmap;
    int capacity;
    int size;
    LinkedList* head;
    LinkedList* tail;

    void removeNode(LinkedList* node) {
        if (node->prev) node->prev->next = node->next;
        if (node->next) node->next->prev = node->prev;
    }

    void addNode(LinkedList* node) {
        node->next = head->next;
        node->prev = head;
        head->next->prev = node;
        head->next = node;
    }

    void evictNode() {
        if (size >= 1) {
            LinkedList* lru = tail->prev;
            hashmap.erase(lru->key);
            removeNode(lru);
            delete lru;
            --size;
        }
    }

public:
    explicit LRUCache(int capacity) {
        this->capacity = capacity;
        this->size = 0;
        this->head = new LinkedList("HEAD", "");
        this->tail = new LinkedList("TAIL", "");
        head->next = tail;
        tail->prev = head;
    }

    ~LRUCache() {
        LinkedList* current = head;
        while (current) {
            LinkedList* next = current->next;
            delete current;
            current = next;
        }
    }

    void print() {
        std::cout<<"Start printing LRU cache----------\n";
        std::cout<<"Print hashmap:\n";
        for (auto it : hashmap) std::cout<<it.first<<": "<<it.second<<"\n";
        LinkedList* curr = head;
        std::cout<<"Print linked list with head = "<<head->val<<", tail = "<<tail->val<<", size = "<<size<<":";
        while (curr != NULL) {
            std::cout<<curr->key<<"="<<curr->val<<" ";
            curr = curr->next;
        }
        std::cout<<"\nPrint reverse\n";
        curr = tail;
        while (curr != NULL) {
            std::cout<<curr->key<<"="<<curr->val<<" ";
            curr = curr->prev;
        }
        std::cout<<"\n";
    }

    std::optional<std::string> get(const std::string& key) {
        if (hashmap.find(key) != hashmap.end()) {
            LinkedList* node = hashmap[key];
            removeNode(node);
            addNode(node);
            return node->val;
        }
        return std::nullopt;
    }

    void put(const std::string& key, const std::string& value) {
        if (hashmap.find(key) != hashmap.end()) {
            LinkedList* node = hashmap[key];
            node->val = value;
            removeNode(node);
            addNode(node);
        } else {
            LinkedList* newNode = new LinkedList(key, value);
            if (size == capacity) {
                evictNode();
            }
            hashmap[key] = newNode;
            addNode(newNode);
            ++size;
        }
    }

    bool exists(const std::string& key) const {
        return hashmap.find(key) != hashmap.end();
    }
};
