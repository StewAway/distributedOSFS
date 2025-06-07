#include <grpcpp/grpcpp.h>
#include "kernel.grpc.pb.h"
#include "syscall_table.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using kernel::KernelSyscall;
using kernel::SyscallRequest;
using kernel::SyscallResponse;

class KernelServiceImpl final : public KernelSyscall::Service {
    Status HandleSyscall(ServerContext* context, const SyscallRequest* request, SyscallResponse* reply) override {
        std::string syscall = request->syscall();
        std::vector<std::string> args(request->args().begin(), request->args().end());

        try {
            std::string result = dispatch_syscall(syscall, args);
            reply->set_status(0);
            reply->set_result(result);
        } catch (const std::exception& e) {
            reply->set_status(1);
            reply->set_error(e.what());
        }
        return Status::OK;
    }
};

void RunServer() {
    std::string server_address("0.0.0.0:50052");
    KernelServiceImpl service;
    ServerBuilder builder;

    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());

    std::cout << "Kernel gRPC server listening on " << server_address << std::endl;
    server->Wait();
}

int main() {
    RunServer();
    return 0;
}
