#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <vector>

enum class WALAction { PUT, DELETE };

struct WALEntry {
    WALAction action;
    std::string key;
    std::string value; // optional for DELETE

    std::string serialize() const;
    static WALEntry deserialize(const std::string& line);
};

class WAL {
public:
    WAL(const std::string& filename);
    ~WAL();

    void appendPut(const std::string& key, const std::string& value);
    void appendDelete(const std::string& key);
    std::vector<WALEntry> recover();

private:
    std::ofstream logfile_;
    std::mutex mutex_;
};
