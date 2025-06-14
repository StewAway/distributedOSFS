// fs_server.cpp
#include <grpcpp/grpcpp.h>
#include "filesystem.grpc.pb.h"
#include "sfs.h"
#include <iostream>
#include <cstdlib>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
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

class FileSystemServiceImpl final : public FileSystem::Service {
public:
    FileSystemServiceImpl() {
        // Initialize the underlying Simple File System
        sfs_init(); 
        std::cout << "[fs_server] SFS initialized successfully.\n";
    }

    Status Create(ServerContext* ctx, const CreateRequest* req, CreateResponse* res) override {
        int inum = sfs_create(req->path());
        if (inum < 0) res->set_error("Create failed");
        else          res->set_inum(inum);
        return Status::OK;
    }

    Status Mkdir(ServerContext* ctx, const MkdirRequest* req, MkdirResponse* res) override {
        int inum = sfs_mkdir(req->path());
        if (inum < 0) res->set_error("Mkdir failed");
        else          res->set_inum(inum);
        return Status::OK;
    }

    Status Open(ServerContext* ctx, const OpenRequest* req, OpenResponse* res) override {
        int fd = sfs_open(req->filename());
        if (fd < 0) res->set_error("Open failed");
        else        res->set_fd(fd);
        return Status::OK;
    }

    Status Write(ServerContext* ctx, const WriteRequest* req, WriteResponse* res) override {
        int written = sfs_write(req->fd(), req->data().c_str(), req->data().size());
        res->set_success(written >= 0);
        if (written < 0) res->set_error("Write failed");
        return Status::OK;
    }

    Status Read(ServerContext* ctx, const ReadRequest* req, ReadResponse* res) override {
        std::vector<char> buf(req->num_bytes());
        int n = sfs_read(req->fd(), buf.data(), req->num_bytes());
        if (n < 0) {
            res->set_error("Read failed");
        } else {
            res->set_data(std::string(buf.data(), n));
        }
        return Status::OK;
    }

    Status Seek(ServerContext* ctx, const SeekRequest* req, SeekResponse* res) override {
        bool ok = sfs_seek(req->fd(), req->offset(), req->whence());
        res->set_success(ok);
        if (!ok) res->set_error("Seek failed");
        return Status::OK;
    }

    Status Listdir(ServerContext* ctx, const ListdirRequest* req, ListdirResponse* res) override {
        auto names = sfs_listdir(req->path());

        if (names.empty()) {
            res->set_error("Directory not found or not a directory");
        } else {
            for (const auto& n: names) {
                res->add_entries(n);
            }
        }
        return Status::OK;
    }

    Status Remove(ServerContext* ctx, const RemoveRequest* req, RemoveResponse* res) override {
        bool ok = sfs_remove(req->path());
        res->set_success(ok);
        if (!ok) res->set_error("Remove failed");
        return Status::OK;
    }
};

void RunServer(const std::string& address) {
    FileSystemServiceImpl service;  // constructor calls sfs_init()
    ServerBuilder builder;
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "[fs_server] listening on " << address << std::endl;
    server->Wait();
}

int main(int argc, char** argv) {
    const std::string address = "0.0.0.0:50051";
    RunServer(address);
    return 0;
}
