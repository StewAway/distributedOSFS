#include <grpcpp/grpcpp.h>
#include "filesystem.grpc.pb.h"
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <memory>

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
using fs::SeekReply;
using fs::MkdirRequest;
using fs::MkdirResponse;
using fs::RemoveRequest;
using fs::RemoveResponse;
using fd::ListdirRequest;
using fs::ListdirResponse;

class FileSystemServiceImpl final : public FileSystem::Service {
    Status Open(ServerContext* context, const OpenRequest* req, const OpenResponse* res) override {
            
    }
    Status Write(ServerContext* context, const WriteRequest* req, const WriteReply* res) override {
    
    }
    Status Read(ServerContext* context, const ReadRequest* req, const ReadRequest* res) override {
    
    }
    Status Seek(ServerContext* context, const SeekRequest* req, const SeekResponse* res) override {
    
    }
    Status Mkdir(ServerContest* context, const MkdirRequest* req, MkdirResponse* res) override {
    
    }
    Status Remove(ServerContext* context, const RemoveRequest* req, RemoveResponse* res) override {
    
    }
    Status Listdir(ServerContext* context const ListdirRequest* req, ListdirResponse* res) override {
    
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
