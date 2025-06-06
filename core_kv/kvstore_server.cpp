#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>

#include <grpcpp/grpcpp.h>
#include "kvstore.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using kvstore::KVStore;
using kvstore::PutRequest;
using kvstore::PutReply;
using kvstore::GetRequest;
using kvstore::GetReply;
using kvstore::DeleteRequest;
using kvstore::DeleteReply;

class KVStoreServiceImpl final : public KVStore::Service {
    private:
	    std::unordered_map<std::string, std::string> store_;
	    std::mutex mutex_; // thread safety
	public:
	    Status Put(ServerContext* context, const PutRequest* request, PutReply* reply) override {
		    std::lock_guard<std::mutex> lock(mutex_);
			store_[request->key()] = request->value();
			std::cout<<"[Put] "<<request->key()<<" => "<<request->value()<<"\n";
			reply->set_success(true);
			return Status::OK;			
		}

		Status Get(ServerContext* context, const GetRequest* request, GetReply* reply) override {
	        std::lock_guard<std::mutex> lock(mutex_);
		    auto it = store_.find(request->key());
	        if (it != store_.end()) { 
		        reply->set_found(true);
			    reply->set_value(it->second);
			    std::cout<<"[Get] "<<request->key()<<" => "<<it->second<<"\n";
			} else {
			    reply->set_found(false);
			    std::cout<<"[Get] "<<request->key()<<" not Found.\n";
		    }
			return Status::OK;
		}
		Status Delete(ServerContext* context, const DeleteRequest* request, DeleteReply* reply) override {
	        std::lock_guard<std::mutex> lock(mutex_);
		  	size_t erased = store_.erase(request->key());
	        reply->set_success(erased > 0);
	        std::cout<<"[Delete] "<<request->key()<<(erased ? " deleted" : " not found")<<"\n";
	        return Status::OK;
		}
};

void RunServer() {
    std::string address("0.0.0.0:50051");

    KVStoreServiceImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout<<"KVStore Server listening on "<<address<<"\n";

    server->Wait();
}

int main() {
    RunServer();
    return 0;
}
