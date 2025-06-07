#include <iostream>
#include "wal.h"

int main() {
    std::string log_filename = "test_wal.log";

    // Step 1: Create a WAL and append some entries
    {
        WAL wal(log_filename);
        wal.appendPut("foo", "123");
        wal.appendPut("bar", "456");
        wal.appendDelete("foo");
        wal.appendPut("baz", "789");
        std::cout << "[Write] WAL entries written.\n";
    }

    // Step 2: Reopen the WAL and recover entries
    {
        WAL wal(log_filename);
        auto entries = wal.recover();

        std::cout << "[Recover] Recovered " << entries.size() << " entries:\n";
        for (const auto& entry : entries) {
            if (entry.action == WALAction::PUT) {
                std::cout << "  PUT " << entry.key << " = " << entry.value << "\n";
            } else {
                std::cout << "  DELETE " << entry.key << "\n";
            }
        }
    }

    return 0;
}
