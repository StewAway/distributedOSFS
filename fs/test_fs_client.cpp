// test_fs_client.cpp
#include <grpcpp/grpcpp.h>
#include "filesystem.grpc.pb.h"
#include <iostream>
#include <vector>
#include <string>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using fs::FileSystem;
using fs::CreateRequest;
using fs::CreateResponse;
using fs::MkdirRequest;
using fs::MkdirResponse;
using fs::OpenRequest;
using fs::OpenResponse;
using fs::WriteRequest;
using fs::WriteResponse;
using fs::ReadRequest;
using fs::ReadResponse;
using fs::SeekRequest;
using fs::SeekResponse;
using fs::ListdirRequest;
using fs::ListdirResponse;
using fs::RemoveRequest;
using fs::RemoveResponse;

class FSClient {
  std::unique_ptr<FileSystem::Stub> stub_;
public:
  FSClient(std::shared_ptr<Channel> channel)
    : stub_(FileSystem::NewStub(channel)) {}

  int Create(const std::string& path) {
    CreateRequest  req; CreateResponse res; ClientContext ctx;
    req.set_path(path);
    Status s = stub_->Create(&ctx, req, &res);
    if (!s.ok()) {
      std::cerr << "RPC Create failed: " << s.error_message() << "\n";
      return -1;
    }
    if (!res.error().empty()) {
      std::cerr << "Create error: " << res.error() << "\n";
      return -1;
    }
    return res.inum();
  }

  int Mkdir(const std::string& path) {
    MkdirRequest  req; MkdirResponse res; ClientContext ctx;
    req.set_path(path);
    Status s = stub_->Mkdir(&ctx, req, &res);
    if (!s.ok()) {
      std::cerr << "RPC Mkdir failed: " << s.error_message() << "\n";
      return -1;
    }
    if (!res.error().empty()) {
      std::cerr << "Mkdir error: " << res.error() << "\n";
      return -1;
    }
    return res.inum();
  }

  int Open(const std::string& path) {
    OpenRequest  req; OpenResponse res; ClientContext ctx;
    req.set_filename(path);
    Status s = stub_->Open(&ctx, req, &res);
    if (!s.ok()) {
      std::cerr << "RPC Open failed: " << s.error_message() << "\n";
      return -1;
    }
    if (!res.error().empty()) {
      std::cerr << "Open error: " << res.error() << "\n";
      return -1;
    }
    return res.fd();
  }

  bool Write(int fd, const std::string& data) {
    WriteRequest  req; WriteResponse res; ClientContext ctx;
    req.set_fd(fd);
    req.set_data(data);
    Status s = stub_->Write(&ctx, req, &res);
    if (!s.ok()) {
      std::cerr << "RPC Write failed: " << s.error_message() << "\n";
      return false;
    }
    if (!res.error().empty()) {
      std::cerr << "Write error: " << res.error() << "\n";
      return false;
    }
    return res.success();
  }

  std::string Read(int fd, int count) {
    ReadRequest  req; ReadResponse res; ClientContext ctx;
    req.set_fd(fd);
    req.set_num_bytes(count);
    Status s = stub_->Read(&ctx, req, &res);
    if (!s.ok()) {
      std::cerr << "RPC Read failed: " << s.error_message() << "\n";
      return {};
    }
    if (!res.error().empty()) {
      std::cerr << "Read error: " << res.error() << "\n";
      return {};
    }
    return res.data();
  }

  bool Seek(int fd, int offset, int whence) {
    SeekRequest  req; SeekResponse res; ClientContext ctx;
    req.set_fd(fd);
    req.set_offset(offset);
    req.set_whence(whence);
    Status s = stub_->Seek(&ctx, req, &res);
    if (!s.ok()) {
      std::cerr << "RPC Seek failed: " << s.error_message() << "\n";
      return false;
    }
    if (!res.success()) {
      std::cerr << "Seek error: " << res.error() << "\n";
      return false;
    }
    return true;
  }

  std::vector<std::string> Listdir(const std::string& path) {
    ListdirRequest  req; ListdirResponse res; ClientContext ctx;
    req.set_path(path);
    Status s = stub_->Listdir(&ctx, req, &res);
    std::vector<std::string> out;
    if (!s.ok()) {
      std::cerr << "RPC ListDir failed: " << s.error_message() << "\n";
      return out;
    }
    if (!res.error().empty()) {
      std::cerr << "ListDir error: " << res.error() << "\n";
      return out;
    }
    for (const auto &name : res.entries()) {
      out.push_back(name);
    }
    return out;
  }

  bool Remove(const std::string& path) {
    RemoveRequest  req; RemoveResponse res; ClientContext ctx;
    req.set_path(path);
    Status s = stub_->Remove(&ctx, req, &res);
    if (!s.ok()) {
      std::cerr << "RPC Remove failed: " << s.error_message() << "\n";
      return false;
    }
    if (!res.success()) {
      std::cerr << "Remove error: " << res.error() << "\n";
      return false;
    }
    return true;
  }
};

int main() {
  FSClient client(grpc::CreateChannel(
    "localhost:50051", grpc::InsecureChannelCredentials()));

  // 1) mkdir /testdir
  std::cout << "Mkdir /testdir → inum=" << client.Mkdir("/testdir") << "\n";

  // 2) create file
  std::cout << "Create file → inum=" 
            << client.Create("/testdir/hello.txt") << "\n";

  // 3) open file
  int fd = client.Open("/testdir/hello.txt");
  std::cout << "Open file → fd=" << fd << "\n";

  // 4) write
  bool w = client.Write(fd, "Hello, SFS!");
  std::cout << "Write → " << (w ? "OK" : "FAIL") << "\n";

  // 5) seek back
  bool sk = client.Seek(fd, 0, 0 /*SEEK_SET*/);
  std::cout << "Seek → " << (sk ? "OK" : "FAIL") << "\n";

  // 6) read 5 bytes
  auto s = client.Read(fd, 5);
  std::cout << "Read 5 bytes → '" << s << "'\n";

  // 7) list contents
  auto list1 = client.Listdir("/testdir");
  std::cout << "List /testdir:\n";
  for (auto &name : list1) std::cout << " - " << name << "\n";

  // 8) remove file
  std::cout << "Remove file → " 
            << (client.Remove("/testdir/hello.txt") ? "OK" : "FAIL")
            << "\n";

  // 9) list again
  auto list2 = client.Listdir("/testdir");
  std::cout << "List /testdir after remove: size=" << list2.size() << "\n";

  // 10) remove directory
  std::cout << "Remove dir → " 
            << (client.Remove("/testdir") ? "OK" : "FAIL") << "\n";

  // 11) root listing
  auto root = client.Listdir("/");
  std::cout << "List /:\n";
  for (auto &name : root) std::cout << " - " << name << "\n";

  return 0;
}
