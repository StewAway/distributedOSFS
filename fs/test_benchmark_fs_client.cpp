// ========== File: test_benchmark_fs_client.cpp ==========
#include <grpcpp/grpcpp.h>
#include "filesystem.grpc.pb.h"
#include <iostream>
#include <chrono>
#include <random>

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
  FSClient(std::shared_ptr<Channel> ch) : stub_(FileSystem::NewStub(ch)) {}

  int Mount(const std::string& img, bool use_cache = true) {
    MountRequest req; req.set_disk_image(img); req.set_enable_cache(use_cache);
    MountResponse res; ClientContext ctx;
    stub_->Mount(&ctx, req, &res);
    return res.mount_id();
  }

  int Create(int mid, const std::string& path) {
    FileRequest req; req.set_mount_id(mid); req.set_path(path);
    CreateResponse res; ClientContext ctx;
    stub_->Create(&ctx, req, &res);
    return res.inum();
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

  void Read(int mid, int fd, int nbytes) {
    ReadRequestMulti req;
    req.set_mount_id(mid);
    req.set_fd(fd);
    req.set_num_bytes(nbytes);
    ReadResponse res; ClientContext ctx;
    stub_->Read(&ctx, req, &res);
  }
};

void run_benchmark(bool use_cache, const std::string& disk_image) {
  auto channel = grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());
  FSClient client(channel);

  int mid = client.Mount(disk_image, use_cache);
  client.Create(mid, "/bench.txt");
  int fd = client.Open(mid, "/bench.txt");

  std::string data(4096, 'A');
  int ops = 100000;

  auto start = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < ops; ++i) {
    if (i % 2 == 0) {
        client.Write(mid, fd, data);
        //std::cout<<use_cache<<" "<<"write "<<i<<"\n";
    }
    else {
        client.Read(mid, fd, 4096);
        //std::cout<<use_cache<<" "<<"read "<<i<<"\n";
    }
  }

  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

  std::cout << (use_cache ? "[With Cache] " : "[No Cache] ")
            << "Total time for " << ops << " operations: " << duration << " ms\n";
}

int main() {
  std::cout << "Running benchmark without cache...\n";
  run_benchmark(false, "disk_nocache.img");

  std::cout << "Running benchmark with cache...\n";
  run_benchmark(true, "disk_cache.img");

  return 0;
}
