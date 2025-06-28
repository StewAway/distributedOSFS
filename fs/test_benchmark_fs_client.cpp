// ========== File: test_benchmark_fs_client.cpp ==========
#include <grpcpp/grpcpp.h>
#include "filesystem.grpc.pb.h"
#include <iostream>
#include <chrono>
#include <random>
#include <vector>
#include <iomanip>
#include <cmath>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using fs::FileSystem;
using fs::MountRequest;
using fs::MountResponse;
using fs::FileRequest;
using fs::CreateResponse;
using fs::OpenResponse;
using fs::WriteRequestMulti;
using fs::WriteResponse;
using fs::ReadRequestMulti;
using fs::ReadResponse;

class FSClient {
  std::unique_ptr<FileSystem::Stub> stub_;
public:
  FSClient(std::shared_ptr<Channel> ch)
    : stub_(FileSystem::NewStub(ch)) {}

  int Mount(const std::string& img, bool use_cache, int cache_blocks = 0) {
    MountRequest req;
    req.set_disk_image(img);
    req.set_enable_cache(use_cache);
    req.set_cache_blocks(cache_blocks);
    MountResponse res;
    ClientContext ctx;
    stub_->Mount(&ctx, req, &res);
    return res.mount_id();
  }

  void Create(int mid, const std::string& path) {
    FileRequest req; req.set_mount_id(mid); req.set_path(path);
    CreateResponse res; ClientContext ctx;
    stub_->Create(&ctx, req, &res);
  }

  int Open(int mid, const std::string& path) {
    FileRequest req; req.set_mount_id(mid); req.set_path(path);
    OpenResponse res; ClientContext ctx;
    stub_->Open(&ctx, req, &res);
    return res.fd();
  }

  void Write(int mid, int fd, const std::string& data) {
    WriteRequestMulti req;
    req.set_mount_id(mid);
    req.set_fd(fd);
    req.set_data(data);
    WriteResponse res; ClientContext ctx;
    stub_->Write(&ctx, req, &res);
  }

  std::string Read(int mid, int fd, int nbytes) {
    ReadRequestMulti req;
    req.set_mount_id(mid);
    req.set_fd(fd);
    req.set_num_bytes(nbytes);
    ReadResponse res; ClientContext ctx;
    stub_->Read(&ctx, req, &res);
    return res.data();
  }
};

// Simple timer helper
static double timed_run(std::function<void()> fn) {
    auto start = std::chrono::high_resolution_clock::now();
    fn();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double>(end - start).count();
}

int main() {
    const std::string disk = "disk_benchmark.img";
    const int NUM_OPS = 50000;
    const int BLOCK_SIZE = 4096;
    const int SMALL_IO = 512;
    const int LARGE_IO = 16384;
    
    FSClient client(grpc::CreateChannel(
        "localhost:50051", grpc::InsecureChannelCredentials()));

    struct Pattern {
        std::string name;
        bool use_cache;
        int cache_blocks;
        int io_size;
        bool sequential;
    };

    std::vector<Pattern> tests = {
        {"ColdRandom512", false, 0, SMALL_IO, false},
        {"WarmRandom512",  true, 1024, SMALL_IO, false},
        {"ColdSeq4K",    false, 0, BLOCK_SIZE, true},
        {"WarmSeq4K",    true, 1024, BLOCK_SIZE, true},
        {"ColdSeq16K",   false, 0, LARGE_IO, true},
        {"WarmSeq16K",   true, 1024, LARGE_IO, true}
    };

    std::cout << std::fixed << std::setprecision(3);
    std::cout << "Test           Time(s)\n";
    for (auto &p : tests) {
        int mid = client.Mount(disk, p.use_cache, p.cache_blocks);
        client.Create(mid, "/bench.txt");
        int fd = client.Open(mid, "/bench.txt");

        int total_ops = NUM_OPS;
        std::string buf(p.io_size, 'X');
        std::mt19937_64 rng(42);
        std::uniform_int_distribution<int> dist(0, 1023);

        double t = timed_run([&]{
            for (int i = 0; i < total_ops; ++i) {
                int offset_index = p.sequential ? (i % 1024) : dist(rng);
                // Use file offset via repeated writes or reads
                if (i & 1) {
                    client.Read(mid, fd, p.io_size);
                } else {
                    client.Write(mid, fd, buf);
                }
            }
        });

        std::cout << std::setw(12) << p.name << " "
                  << std::setw(7) << t << "\n";
    }

    // Theoretical speedup calculation:
    // Assume disk I/O ~5ms per block vs mem ~0.1ms
    double disk_ms = 5.0;
    double mem_ms  = 0.1;
    double speedup = disk_ms / mem_ms;
    std::cout << "\nTheoretical max speedup (disk vs mem): "
              << speedup << "x" << std::endl;

    return 0;
}
