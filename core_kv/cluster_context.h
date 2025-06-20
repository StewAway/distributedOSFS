#ifndef CLUSTER_CONTEXT_H
#define CLUSTER_CONTEST_H

#include <string>
#include <vector>

struct ClusterContext {
    int node_id;
    bool is_leader;
    std::string self_address;
    std::vector<std::string> peer_addresses;

    static ClusterContext& Get() {
        static ClusterContext instance;
        return instance;
    }
private:
    ClusterContext() = default;
};
