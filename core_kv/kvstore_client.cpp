#include <iostream>
#include <memory>
#include <string>
#include <grpcpp/grpcpp.h>
#include "kvstore.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using kvstore::KVStore;
using kvstore::PutRequest;
using kvstore::PutReply;
using kvstore::GetRequest;
using kvstore::GetReply;
using kvstore::DeleteRequest;
using kvstore::DeleteReply;

std::string extract_flag_value(const std::string& arg, const std::string& prefix) {
    if (arg.rfind(prefix, 0) == 0) {
        return arg.substr(prefix.size());
    }
    return "";
}

class KVStoreClient {
public:
    KVStoreClient(std::shared_ptr<Channel> channel)
        : stub_(KVStore::NewStub(channel)) {}

    void Put(const std::string& key, const std::string& value) {
        PutRequest req;
        req.set_key(key);
        req.set_value(value);

        PutReply rep;
        ClientContext context;
        Status status = stub_->Put(&context, req, &rep);
        if (status.ok() && rep.success()) {
            std::cout<<"[Client] Put success\n";
        } else {
            std::cout<<"[Client] Put failed\n";
        }
    }

    void Get(const std::string& key) {
        GetRequest request;
        request.set_key(key);

        GetReply reply;
        ClientContext context;

        Status status = stub_->Get(&context, request, &reply);

        if (status.ok()) {
            std::cout << "[Get OK] key: " << key << ", value: " << reply.value() << std::endl;
        } else {
            std::cerr << "[Get Failed] " << status.error_message() << std::endl;
        }
    }

private:
    std::unique_ptr<KVStore::Stub> stub_;
};

int main(int argc, char** argv) {
    std::string port = "50051";

    // Parse flags
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (auto val = extract_flag_value(arg, "--port="); !val.empty()) {
            port = val;
        }
    }

    std::string target = "localhost:" + port;
    std::cout << "[Client] Connecting to server at " << target << "\n";

    KVStoreClient client(grpc::CreateChannel(target, grpc::InsecureChannelCredentials()));

    // Example test sequence
    for (int i = 1; i <= 6; ++i) {
        client.Put("k" + std::to_string(i), "v" + std::to_string(i));
    }

    for (int i = 1; i <= 6; ++i) {
        client.Get("k" + std::to_string(i));
    }

    return 0;
}
