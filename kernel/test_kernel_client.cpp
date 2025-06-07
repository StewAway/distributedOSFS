#include <grpcpp/grpcpp.h>
#include "kernel.grpc.pb.h"
#include <iostream>

using kernel::KernelSyscall;
using kernel::SyscallRequest;
using kernel::SyscallResponse;

int main() {
    auto channel = grpc::CreateChannel("localhost:50052", grpc::InsecureChannelCredentials());
    std::unique_ptr<KernelSyscall::Stub> stub = KernelSyscall::NewStub(channel);

    SyscallRequest req;
    req.set_syscall("open");
    req.add_args("myfile.txt");

    SyscallResponse res;
    grpc::ClientContext ctx;

    grpc::Status status = stub->HandleSyscall(&ctx, req, &res);

    if (status.ok()) {
        if (res.status() == 0)
            std::cout << "Syscall result: " << res.result() << std::endl;
        else
            std::cerr << "Error: " << res.error() << std::endl;
    } else {
        std::cerr << "RPC failed: " << status.error_message() << std::endl;
    }

    return 0;
}
