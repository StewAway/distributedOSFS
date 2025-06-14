#include <grpcpp/grpcpp.h>
#include "filesystem.grpc.pb.h"
#include <iostream>
#include "sfs.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using fs::FileSystem;
using fs::OpenRequest;
using fs::OpenResponse;
using fs::ReadRequest;
using fs::ReadResponse;
using fs::WriteRequest;
using fs::WriteResponse;
using fs::SeekRequest;
using fs::SeekResponse;
using fs::MkdirRequest;
using fs::MkdirResponse;
using fs::RemoveRequest;
using fs::RemoveResponse;
using fs::ListdirRequest;
using fs::ListdirResponse;

class FileSystemServiceImpl final : public FileSystem::Service {
public:
    FileSystemServiceImpl() {
        sfs_init();
    }

    Status Open(ServerContext* context, const OpenRequest* req, OpenResponse* res) override {
        int fd = sfs_open(req->filename());
        if (fd < 0) res->set_error("Open failed");
        else res->set_fd(fd);
        return Status::OK;
    }

    Status Write(ServerContext* context, const WriteRequest* req, WriteResponse* res) override {
        int n = sfs_write(req->fd(), req->data().data(), req->data().size());
        if (n < 0) {
            res->set_success(false);
            res->set_error("Write failed");
        } else {
            res->set_success(true);
        }
        return Status::OK;
    }

    Status Read(ServerContext* context, const ReadRequest* req, ReadResponse* res) override {
        char buf[req->num_bytes()] = {0};
        int n = sfs_read(req->fd(), buf, req->num_bytes());
        if (n < 0) res->set_error("Read failed");
        else res->set_data(std::string(buf, n));
        return Status::OK;
    }

    Status Seek(ServerContext* context, const SeekRequest* req, SeekResponse* res) override {
        bool ok = sfs_seek(req->fd(), req->offset(), req->whence());
        res->set_success(ok);
        if (!ok) res->set_error("Seek failed");
        return Status::OK;
    }

    Status Mkdir(ServerContext* context, const MkdirRequest* req, MkdirResponse* res) override {
        int inum = sfs_mkdir(req->path());
        if (inum < 0) res->set_error("Mkdir failed");
        else res->set_inum(inum);
        return Status::OK;
    }

    Status Remove(ServerContext* context, const RemoveRequest* req, RemoveResponse* res) override {
        int r = sfs_remove(req->path());
        res->set_success(r == 0);
        if (r < 0) res->set_error("Remove failed");
        return Status::OK;
    }

    Status Listdir(ServerContext* context, const ListdirRequest* req, ListdirResponse* res) override {
        DirList entries;
        int r = sfs_listdir(req->path(), entries);
        if (r < 0) res->set_error("Listdir failed");
        else {
            for (const auto& e : entries) res->add_entries(e);
        }
        return Status::OK;
    }
};

void RunServer() {
    sfs_init();
    std::string server_address("0.0.0.0:50051");
    FileSystemServiceImpl service;
    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "[fs_server] Running on " << server_address << std::endl;
    server->Wait();
}

int main() {
    RunServer();
    return 0;
}
