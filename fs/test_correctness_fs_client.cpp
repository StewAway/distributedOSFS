// ========== File: test_correctness_fs_client.cpp ==========
#include <grpcpp/grpcpp.h>
#include "filesystem.grpc.pb.h"
#include <iostream>
#include <vector>
#include <string>
#include <cassert>

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
using fs::SeekRequestMulti;
using fs::SeekResponse;
using fs::ListdirResponse;
using fs::RemoveResponse;

class FSClient {
  std::unique_ptr<FileSystem::Stub> stub_;
public:
  FSClient(std::shared_ptr<Channel> ch) : stub_(FileSystem::NewStub(ch)) {}

  int Mount(const std::string& img, bool use_cache = true) {
    MountRequest req; req.set_disk_image(img); req.set_enable_cache(use_cache);
    MountResponse res; ClientContext ctx;
    stub_->Mount(&ctx, req, &res);
    if (!res.error().empty()) {
      std::cerr<<"Mount error: "<<res.error()<<"\n"; return -1;
    }
    return res.mount_id();
  }

  int Create(int mid, const std::string& path) {
    FileRequest req; req.set_mount_id(mid); req.set_path(path);
    CreateResponse res; ClientContext ctx;
    stub_->Create(&ctx, req, &res);
    if (!res.error().empty()) {
      std::cerr<<"Create error: "<<res.error()<<"\n"; return -1;
    }
    return res.inum();
  }

  int Open(int mid, const std::string& path) {
    FileRequest req; req.set_mount_id(mid); req.set_path(path);
    OpenResponse res; ClientContext ctx;
    stub_->Open(&ctx, req, &res);
    if (!res.error().empty()) {
      std::cerr<<"Open error: "<<res.error()<<"\n"; return -1;
    }
    return res.fd();
  }

  bool Write(int mid, int fd, const std::string& data) {
    WriteRequestMulti req;
    req.set_mount_id(mid);
    req.set_fd(fd);
    req.set_data(data);
    WriteResponse res; ClientContext ctx;
    stub_->Write(&ctx, req, &res);
    if (!res.error().empty()) {
      std::cerr<<"Write error: "<<res.error()<<"\n"; return false;
    }
    return res.success();
  }

  std::string Read(int mid, int fd, int n) {
    ReadRequestMulti req;
    req.set_mount_id(mid);
    req.set_fd(fd);
    req.set_num_bytes(n);
    ReadResponse res; ClientContext ctx;
    stub_->Read(&ctx, req, &res);
    if (!res.error().empty()) {
      std::cerr<<"Read error: "<<res.error()<<"\n"; return "";
    }
    return res.data();
  }

  bool Seek(int mid, int fd, int off, int whence) {
    SeekRequestMulti req;
    req.set_mount_id(mid);
    req.set_fd(fd);
    req.set_offset(off);
    req.set_whence(whence);
    SeekResponse res; ClientContext ctx;
    stub_->Seek(&ctx, req, &res);
    if (!res.error().empty()) {
      std::cerr<<"Seek error: "<<res.error()<<"\n"; return false;
    }
    return res.success();
  }

  std::vector<std::string> Listdir(int mid) {
    FileRequest req; req.set_mount_id(mid); req.set_path("/");
    ListdirResponse res; ClientContext ctx;
    stub_->Listdir(&ctx, req, &res);
    if (!res.error().empty()) {
      std::cerr<<"ListDir error: "<<res.error()<<"\n"; return {};
    }
    return { res.entries().begin(), res.entries().end() };
  }

  bool Remove(int mid, const std::string& path) {
    FileRequest req; req.set_mount_id(mid); req.set_path(path);
    RemoveResponse res; ClientContext ctx;
    stub_->Remove(&ctx, req, &res);
    if (!res.error().empty()) {
      std::cerr<<"Remove error: "<<res.error()<<"\n"; return false;
    }
    return res.success();
  }
};

int main() {
  auto channel = grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());
  FSClient client(channel);

  int mid = client.Mount("disk_correctness.img", true);
  if (mid < 0) return 1;
  std::cout<<"Mounted disk_correctness.img as mount_id="<<mid<<"\n";

  client.Create(mid, "/hello.txt");
  int fd = client.Open(mid, "/hello.txt");
  client.Write(mid, fd, "Test1!");
  client.Seek(mid, fd, 0, 0);
  std::string out = client.Read(mid, fd, 6);
  std::cout<<"Read back: "<<out<<"\n";

  auto root = client.Listdir(mid);
  std::cout<<"Root entries:";
  for (auto &n : root) std::cout<<" "<<n;
  std::cout<<"\n";

  client.Remove(mid, "/hello.txt");
  std::cout<<"After remove, root size = "<< client.Listdir(mid).size()<<"\n";

  return 0;
}
