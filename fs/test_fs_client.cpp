// === test_fs_client.cpp — gRPC File System Test Client ===

#include "filesystem.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <iostream>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using fs::FileSystem;

class FSClient {
public:
    FSClient(std::shared_ptr<Channel> channel) : stub(fs::FileSystem::NewStub(channel)) {}

    int open(const std::string& path) {
        fs::OpenRequest req;
        req.set_filename(path);
        fs::OpenResponse res;
        ClientContext ctx;
        stub->Open(&ctx, req, &res);
        return res.f();
    }

    std::string read(int fd, int n) {
        fs::ReadRequest req;
        req.set_fd(fd);
        req.set_num_bytes(n);
        fs::ReadResponse res;
        ClientContext ctx;
        stub->Read(&ctx, req, &res);
        return res.data();
    }

    void write(int fd, const std::string& data) {
        fs::WriteRequest req;
        req.set_fd(fd);
        req.set_data(data);
        fs::WriteResponse res;
        ClientContext ctx;
        stub->Write(&ctx, req, &res);
        std::cout << "Wrote: " << data.size() << " bytes, success: " << res.success() << std::endl;
    }

    void seek(int fd, int offset, int whence) {
        fs::SeekRequest req;
        req.set_fd(fd);
        req.set_offset(offset);
        req.set_whence(whence);
        fs::SeekResponse res;
        ClientContext ctx;
        stub->Seek(&ctx, req, &res);
        std::cout << "Seek: success = " << res.success() << std::endl;
    }

    void mkdir(const std::string& path) {
        fs::MkdirRequest req;
        req.set_path(path);
        fs::MkdirResponse res;
        ClientContext ctx;
        stub->Mkdir(&ctx, req, &res);
        std::cout << "Mkdir: inum = " << res.inum() << ", err: " << res.error() << std::endl;
    }

    void remove(const std::string& path) {
        fs::RemoveRequest req;
        req.set_path(path);
        fs::RemoveResponse res;
        ClientContext ctx;
        stub->Remove(&ctx, req, &res);
        std::cout << "Remove: success = " << res.success() << std::endl;
    }

private:
    std::unique_ptr<fs::FileSystem::Stub> stub;
};

int main() {
    FSClient client(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));

    client.mkdir("/test");
    int fd = client.open("/test/bigfile");

    std::string data(8192, 'A'); // 8KB to trigger indirect blocks
    client.write(fd, data);

    client.seek(fd, 0, 0); // SEEK_SET
    std::string result = client.read(fd, 8192);
    std::cout << "Read " << result.size() << " bytes, first 10: " << result.substr(0, 10) << std::endl;

    client.remove("/test/bigfile");
    return 0;
}

