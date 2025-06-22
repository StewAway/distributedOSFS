#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <chrono>
#include <random>
#include <atomic>

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
using kvstore::Void;

constexpr int TOTAL_KEYS = 1'000'000;
constexpr int PRELOAD_KEYS = 500'000;
constexpr int TOTAL_OPS = 10'000'000;
constexpr int THREADS = 4;
constexpr double PUT_RATIO = 0.10;
constexpr size_t BLOB_SIZE = 128;

class ZipfGenerator {
public:
    ZipfGenerator(int n, double s) : n_(n), s_(s) {
        init();
    }

    int next() {
        double u = dist_(gen_);
        auto it = std::lower_bound(cdf_.begin(), cdf_.end(), u);
        return std::distance(cdf_.begin(), it);
    }

private:
    int n_;
    double s_;
    std::vector<double> cdf_;
    std::mt19937 gen_{std::random_device{}()};
    std::uniform_real_distribution<> dist_{0.0, 1.0};

    void init() {
        double sum = 0.0;
        for (int i = 1; i <= n_; ++i) sum += 1.0 / std::pow(i, s_);
        double norm = 0.0;
        for (int i = 1; i <= n_; ++i) {
            norm += 1.0 / std::pow(i, s_);
            cdf_.push_back(norm / sum);
        }
    }
};

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
        Void req, res;
        ClientContext ctx;
        stub_->PrintStats(&ctx, req, &res);
    }

private:
    std::unique_ptr<KVStore::Stub> stub_;
};

void worker(KVStoreClient& client, ZipfGenerator& zipf, const std::string& blob, int ops_per_thread, std::atomic<int>& completed_ops) {
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<> op_dist(0.0, 1.0);

    for (int i = 0; i < ops_per_thread; ++i) {
        int key = zipf.next();
        std::string key_str = "key" + std::to_string(key);
        if (op_dist(rng) < PUT_RATIO) {
            client.Put(key_str, blob);
        } else {
            std::string out;
            client.Get(key_str, out);
        }
        ++completed_ops;
    }
}

int main() {
    std::string server_address = "localhost:50051";
    KVStoreClient client(grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()));
    ZipfGenerator zipf(TOTAL_KEYS, 1.0);
    std::string blob(BLOB_SIZE, 'X');

    // Preload
    for (int i = 0; i < PRELOAD_KEYS; ++i) {
        client.Put("key" + std::to_string(i), blob);
    }
    std::cout << "[Preload] Inserted " << PRELOAD_KEYS << " keys.\n";

    // Benchmark
    std::atomic<int> completed_ops{0};
    auto start = std::chrono::high_resolution_clock::now();
    std::vector<std::thread> threads;
    for (int i = 0; i < THREADS; ++i) {
        threads.emplace_back(worker, std::ref(client), std::ref(zipf), std::ref(blob), TOTAL_OPS / THREADS, std::ref(completed_ops));
    }
    for (auto& t : threads) t.join();
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double seconds = duration / 1e9;
    double throughput = TOTAL_OPS / seconds;
    double avg_latency_ns = duration / TOTAL_OPS;

    std::cout << "[Benchmark] Completed " << completed_ops.load() << " operations.\n";
    std::cout << "[Benchmark] Throughput: " << throughput << " ops/sec\n";
    std::cout << "[Benchmark] Avg Latency: " << avg_latency_ns << " ns/op\n";

    std::cout << "\n[Stats from Server]" << std::endl;
    client.PrintStats();

    return 0;
}
