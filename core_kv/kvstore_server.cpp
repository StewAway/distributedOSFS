#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>

#include <grpcpp/grpcpp.h>
#include "kvstore.grpc.pb.h"

#include "wal.h"
WAL wal("wal.log");

#include "lru_cache.h"

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

struct ServerConfig {
    int cache_capacity = 1000;
    std::string port = "50051";
    std::string log_file = "wal.log";
    // Add more as needed
};


/* PARSING EXECUTABLE FLAGS */ 
std::string extract_flag_value(const std::string& arg, const std::string& prefix) {
    if (arg.rfind(prefix, 0) == 0) {
        return arg.substr(prefix.size());
    }
    return "";
}

void parse_flags(int argc, char** argv, ServerConfig& config) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (auto val = extract_flag_value(arg, "--cache_capacity="); !val.empty()) {
            config.cache_capacity = std::stoi(val);
        } else if (auto val = extract_flag_value(arg, "--port="); !val.empty()) {
            config.port = val;
        } else if (auto val = extract_flag_value(arg, "--log_file="); !val.empty()) {
            config.log_file = val;
        }
    }
}

class KVStoreServiceImpl final : public KVStore::Service {
    private:
	    std::unordered_map<std::string, std::string> store_;
	    std::mutex mutex_; // thread safety
	    WAL wal_;
        LRUCache cache_;
    public:
        
        explicit KVStoreServiceImpl(std::string log_file, int cache_capacity) : wal_(log_file), cache_(cache_capacity) {
            recoverFromLog();
        }

        void recoverFromLog() {
            auto logs = wal_.recover();
            for (const auto& entry : logs) {
                if (entry.action == WALAction::PUT) {
                    store_[entry.key] = entry.value;
                    cache_.put(entry.key, entry.value);
                } else {
                    store_.erase(entry.key); 
                }
            }
            std::cout<<"[WAL] Recovery complete: "<<logs.size()<<" entries replayed.\n";
        }

	    Status Put(ServerContext* context, const PutRequest* request, PutReply* reply) override {
		    std::lock_guard<std::mutex> lock(mutex_);
			wal_.appendPut(request->key(), request->value());
            store_[request->key()] = request->value();
            cache_.put(request->key(), request->value());
			std::cout<<"[Put] "<<request->key()<<" => "<<request->value()<<"\n";
			reply->set_success(true);
			return Status::OK;			
		}

		Status Get(ServerContext* context, const GetRequest* request, GetReply* reply) override {
	        std::lock_guard<std::mutex> lock(mutex_);
            //std::cout<<"[Get] "<<request->key()<<"||||||||||||||||\n";
            //cache_.print();
            auto hit = cache_.get(request->key());
            if (hit) {
                reply->set_found(true);
                reply->set_value(*hit);
                std::cout<<"[CacheHit]"<<request->key()<<"\n";
            } else {
		        auto it = store_.find(request->key());
	            if (it != store_.end()) { 
		            reply->set_found(true);
			        reply->set_value(it->second);
                    cache_.put(request->key(), it->second);
			        std::cout<<"[StoreHit] "<<request->key()<<" => "<<it->second<<"\n";
			    } else {
			        reply->set_found(false);
			        std::cout<<"[Miss] "<<request->key()<<" not Found.\n";
		        }
            }
			return Status::OK;
		}
		Status Delete(ServerContext* context, const DeleteRequest* request, DeleteReply* reply) override {
	        std::lock_guard<std::mutex> lock(mutex_);
            wal_.appendDelete(request->key());
		  	size_t erased = store_.erase(request->key());
	        reply->set_success(erased > 0);
	        std::cout<<"[Delete] "<<request->key()<<(erased ? " deleted" : " not found")<<"\n";
	        return Status::OK;
		}
};

int main(int argc, char** argv) {
    ServerConfig config;
    parse_flags(argc, argv, config);

    std::string server_address = "0.0.0.0:" + config.port;

    std::cout << "[Info] Starting server on " << server_address
              << " with cache_capacity=" << config.cache_capacity
              << " log_file=" << config.log_file << "\n";

    KVStoreServiceImpl service(config.log_file, config.cache_capacity);  // pass as needed

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());

    std::cout << "[Info] Server listening...\n";
    server->Wait();
    return 0;
}

