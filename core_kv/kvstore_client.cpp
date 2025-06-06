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

class KVStoreClient {
private:
    std::unique_ptr<KVStore::Stub> stub_;

public:
    KVStoreClient(std::shared_ptr<Channel> channel) : stub_(KVStore::NewStub(channel)) {}

    void Put(const std::string&key, const std::string& value) {
        PutRequest request;
        request.set_key(key);
        request.set_value(value);

        PutReply reply;
        ClientContext context;
        Status status = stub_->Put(&context, request, &reply);
        if (status.ok() && reply.success()) {
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

        if (status.ok() && reply.found()) {
            std::cout<<"[Client] Get success: "<<reply.value()<<"\n";
        } else if (status.ok()) {
            std::cout<<"[Client] Key not found\n";
        } else {
            std::cerr<<"[Client] Get RPC failed\n";
        }
    }

    void Delete(const std::string& key) {
        DeleteRequest request;
        request.set_key(key);

        DeleteReply reply;
        ClientContext context;
        Status status = stub_->Delete(&context, request, &reply);

        if (status.ok() && reply.success()) {
            std::cout<<"[Client] Delete success\n";
        } else {
            std::cout<<"[Client] Delete failed\n";
        }
    }
};

int main() {
    KVStoreClient client(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));

    client.Put("foo", "bar");
    client.Get("foo");
    client.Delete("foo");
    client.Get("foo");

    return 0;
}
