// fs_server.cpp
#include <grpcpp/grpcpp.h>
#include "filesystem.grpc.pb.h"
#include "sfs.h"
#include "fs_context.h"
#include <mutex>
#include <memory>
#include <unordered_map>
#include <iostream>
#include <cstdlib>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using fs::FileSystem;

using fs::MountRequest;
using fs::MountResponse;

using fs::FileRequest;
using fs::CreateResponse;
using fs::MkdirResponse;
using fs::OpenResponse;

using fs::WriteRequestMulti;
using fs::WriteResponse;

using fs::ReadRequestMulti;
using fs::ReadResponse;

using fs::SeekRequestMulti;
using fs::SeekResponse;

using fs::ListdirResponse;
using fs::RemoveResponse;

class FileSystemServiceImpl final : public FileSystem::Service {
    std::mutex mu_;
    int next_mount_id_ = 1;
    std::unordered_map<int, std::unique_ptr<FSContext>> contexts_;

    FSContext* get_ctx(int mid) {
        auto it = contexts_.find(mid);
        return it == contexts_.end() ? nullptr : it->second.get();
    }
public:
    Status Mount(ServerContext*, const MountRequest* req, MountResponse* res) override {
        std::lock_guard<std::mutex> lk(mu_);
        
        int id = next_mount_id_++;
        auto ctx = std::make_unique<FSContext>();
    
        if (!sfs_init(*ctx, req->disk_image())) {
            res->set_error("Failed to initialize FS on " + req->disk_image());
            return Status::OK;
        }
        
        ctx->use_cache = req->enable_cache();
        if (ctx.use_cache) {
            const size_t block_size = BLOCK_SIZE;
            const size_t cache_blocks = CACHE_NUM_BLOCKS;
            ctx->init_cahce(cache_blocks, block_size);
        } 
        contexts_[id] = std::move(ctx);
        res->set_mount_id(id);
        return Status::OK;
    }

    Status Create(ServerContext*, const FileRequest* req, CreateResponse* res) override {
        auto ctx = get_ctx(req->mount_id());
        if (!ctx) {
            res->set_error("Invalid mount_id"); 
            return Status::OK; 
        }
        
        int inum = sfs_create(*ctx, req->path());
        if (inum < 0) res->set_error("Create failed");
        else          res->set_inum(inum);
        return Status::OK;
    }

    Status Mkdir(ServerContext*, const FileRequest* req, MkdirResponse* res) override {
        auto ctx = get_ctx(req->mount_id());
        if (!ctx) {
            res->set_error("Invalid mount_id");
            return Status::OK;
        }
        int inum = sfs_mkdir(*ctx, req->path());
        if (inum < 0) res->set_error("Mkdir failed");
        else          res->set_inum(inum);
        return Status::OK;
    }

    Status Open(ServerContext*, const FileRequest* req, OpenResponse* res) override {
        auto ctx = get_ctx(req->mount_id());
        if (!ctx) {
            res->set_error("Invalid mount_id");
            return Status::OK;
        }
        int fd = sfs_open(*ctx, req->path());
        if (fd < 0) res->set_error("Open failed");
        else        res->set_fd(fd);
        return Status::OK;
    }

    Status Write(ServerContext*, const WriteRequestMulti* req, WriteResponse* res) override {
        auto ctx = get_ctx(req->mount_id());
        if (!ctx) {
            res->set_error("Invalid mount_id");
            return Status::OK;
        }
        int written = sfs_write(*ctx, req->fd(), req->data().c_str(), req->data().size());
        res->set_success(written >= 0);
        if (written < 0) res->set_error("Write failed");
        return Status::OK;
    }

    Status Read(ServerContext*, const ReadRequestMulti* req, ReadResponse* res) override {
        auto ctx = get_ctx(req->mount_id());
        if (!ctx) {
            res->set_error("Invalid mount_id");
            return Status::OK;
        }
        std::vector<char> buf(req->num_bytes());
        int n = sfs_read(*ctx, req->fd(), buf.data(), req->num_bytes());
        if (n < 0) {
            res->set_error("Read failed");
        } else {
            res->set_data(std::string(buf.data(), n));
        }
        return Status::OK;
    }

    Status Seek(ServerContext*, const SeekRequestMulti* req, SeekResponse* res) override {
        auto ctx = get_ctx(req->mount_id());
        if (!ctx) {
            res->set_error("Invalid mount_id");
            return Status::OK;
        }
        bool ok = sfs_seek(*ctx, req->fd(), req->offset(), req->whence());
        res->set_success(ok);
        if (!ok) res->set_error("Seek failed");
        return Status::OK;
    }

    Status Listdir(ServerContext*, const FileRequest* req, ListdirResponse* res) override {
        auto ctx = get_ctx(req->mount_id());
        if (!ctx) {
            res->set_error("Invalid mount_id");
            return Status::OK;
        }
        auto names = sfs_listdir(*ctx, req->path());

        for (const auto& n: names) {
            res->add_entries(n);
        }
        return Status::OK;
    }

    Status Remove(ServerContext*, const FileRequest* req, RemoveResponse* res) override {
        auto ctx = get_ctx(req->mount_id());
        if (!ctx) {
            res->set_error("Invalid mount_id");
            return Status::OK;
        }
        bool ok = sfs_remove(*ctx, req->path());
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
