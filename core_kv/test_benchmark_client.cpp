// test_benchmark_client.cpp
#include <iostream>
#include <memory>
#include <string>
#include <chrono>
#include <random>

#include <grpcpp/grpcpp.h>
#include "kvstore.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using kvstore::KVStore;
using kvstore::PutRequest;
using kvstore::PutReply;
using kvstore::GetRequest;
using kvstore::GetReply;

class KVStoreClient {
public:
    KVStoreClient(std::shared_ptr<Channel> channel)
        : stub_(KVStore::NewStub(channel)) {}

    void Put(const std::string& key, const std::string& value) {
        PutRequest request;
        request.set_key(key);
        request.set_value(value);

        PutReply reply;
        ClientContext context;
        stub_->Put(&context, request, &reply);
    }

    bool Get(const std::string& key, std::string& value_out) {
        GetRequest request;
        request.set_key(key);

        GetReply reply;
        ClientContext context;
        Status status = stub_->Get(&context, request, &reply);
        if (status.ok() && reply.found()) {
            value_out = reply.value();
            return true;
        }
        return false;
    }
    void PrintStats() {
        kvstore::Void request, response;
        ClientContext context;
        Status status = stub_->PrintStats(&context, request, &response);
        if (status.ok()) {
            std::cout << "[Client] Server stats printed.\n";
        } else {
            std::cerr << "[Client] Error printing stats: " << status.error_message() << "\n";
        }
    }


private:
    std::unique_ptr<KVStore::Stub> stub_;
};

void benchmark_gets(KVStoreClient& client, int n_keys, int repeats) {
    std::vector<std::string> keys;
    for (int i = 0; i < n_keys; ++i) {
        std::string key = "key" + std::to_string(i);
        std::string val = "val" + std::to_string(i);
        client.Put(key, val);
        keys.push_back(key);
    }

    std::cout << "[Benchmark] Starting " << repeats << " GETs on " << n_keys << " keys...\n";
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, n_keys - 1);

    auto start = std::chrono::high_resolution_clock::now();

    int found = 0;
    for (int i = 0; i < repeats; ++i) {
        std::string result;
        std::string key = keys[dist(rng)];
        if (client.Get(key, result)) ++found;
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;
    
    std::cout << "[Benchmark] Time: " << elapsed.count() << " ms\n";
    std::cout << "[Benchmark] Found: " << found << "/" << repeats << " (" << (100.0 * found / repeats) << "%)\n";
    std::cout << "[Benchmark] Average GET latency: " << (elapsed.count() / repeats) << " ms\n";
    client.PrintStats();
}

int main(int argc, char** argv) {
    std::string server_address = "localhost:50051";
    KVStoreClient client(grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()));

    int n_keys = 10000;
    int repeats = 100000;
    benchmark_gets(client, n_keys, repeats);
    return 0;
}
