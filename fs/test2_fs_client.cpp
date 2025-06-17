// test2_fs_client.cpp
#include <grpcpp/grpcpp.h>
#include "filesystem.grpc.pb.h"
#include <iostream>
#include <vector>
#include <string>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

// gRPC generated types
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

class FSClient {
    std::unique_ptr<FileSystem::Stub> stub_;
public:
    FSClient(std::shared_ptr<Channel> channel)
        : stub_(FileSystem::NewStub(channel)) {}

    int Mount(const std::string& img) {
        MountRequest req; req.set_disk_image(img);
        MountResponse res; ClientContext ctx;
        Status st = stub_->Mount(&ctx, req, &res);
        if (!st.ok() || !res.error().empty()) {
            std::cerr << "Mount(" << img << ") failed: "
                      << (res.error().empty() ? st.error_message() : res.error())
                      << "\n";
            return -1;
        }
        return res.mount_id();
    }

    bool Create(int mid, const std::string& path) {
        FileRequest req; req.set_mount_id(mid); req.set_path(path);
        CreateResponse res; ClientContext ctx;
        stub_->Create(&ctx, req, &res);
        if (!res.error().empty()) {
            std::cerr << "Create("<<path<<") failed: "<<res.error()<<"\n";
            return false;
        }
        return true;
    }

    int Open(int mid, const std::string& path) {
        FileRequest req; req.set_mount_id(mid); req.set_path(path);
        OpenResponse res; ClientContext ctx;
        stub_->Open(&ctx, req, &res);
        if (!res.error().empty()) {
            std::cerr << "Open("<<path<<") failed: "<<res.error()<<"\n";
            return -1;
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
            std::cerr << "Write(fd="<<fd<<") failed: "<<res.error()<<"\n";
            return false;
        }
        return res.success();
    }

    bool Seek(int mid, int fd, int offset, int whence) {
        SeekRequestMulti req;
        req.set_mount_id(mid);
        req.set_fd(fd);
        req.set_offset(offset);
        req.set_whence(whence);
        SeekResponse res; ClientContext ctx;
        stub_->Seek(&ctx, req, &res);
        if (!res.error().empty()) {
            std::cerr << "Seek(fd="<<fd<<") failed: "<<res.error()<<"\n";
            return false;
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
            std::cerr << "Read(fd="<<fd<<") failed: "<<res.error()<<"\n";
            return "";
        }
        return res.data();
    }

    std::vector<std::string> Listdir(int mid) {
        FileRequest req; req.set_mount_id(mid); req.set_path("/");
        ListdirResponse res; ClientContext ctx;
        stub_->Listdir(&ctx, req, &res);
        if (!res.error().empty()) {
            std::cerr << "Listdir failed: "<<res.error()<<"\n";
            return {};
        }
        return { res.entries().begin(), res.entries().end() };
    }
};

int main() {
    auto channel = grpc::CreateChannel(
        "localhost:50051", grpc::InsecureChannelCredentials());
    FSClient client(channel);

    // Mount two separate disk images
    int A = client.Mount("diskA.img");
    int B = client.Mount("diskB.img");
    if (A < 0 || B < 0) return 1;
    std::cout << "Mounted diskA.img as mount_id="<<A
              << ", diskB.img as mount_id="<<B<<"\n";

    // Create same file in both mounts
    client.Create(A, "/foo");
    client.Create(B, "/foo");

    // Open each file descriptor
    int fdA = client.Open(A, "/foo");
    int fdB = client.Open(B, "/foo");
    if (fdA < 0 || fdB < 0) return 1;

    // Write distinct data
    client.Write(A, fdA, "AAA");
    client.Write(B, fdB, "BBB");

    // Seek back to start in both
    client.Seek(A, fdA, 0, 0);  // SEEK_SET
    client.Seek(B, fdB, 0, 0);

    // Read and display
    std::string rA = client.Read(A, fdA, 3);
    std::string rB = client.Read(B, fdB, 3);
    std::cout << "Read from A: " << rA << "\n";
    std::cout << "Read from B: " << rB << "\n";

    // List root of each mount
    auto listA = client.Listdir(A);
    auto listB = client.Listdir(B);
    std::cout << "Root A:"; for (auto &n : listA) std::cout<<" "<<n; std::cout<<"\n";
    std::cout << "Root B:"; for (auto &n : listB) std::cout<<" "<<n; std::cout<<"\n";

    return 0;
}
