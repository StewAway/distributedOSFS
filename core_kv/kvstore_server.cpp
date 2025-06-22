#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>

#include <grpcpp/grpcpp.h>
#include "kvstore.grpc.pb.h"

#include "wal.h"
#include "disk_store.h"
#include "cache_controller.h"
WAL wal("wal.log");

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
    CachePolicy cache_policy = CachePolicy::LRU;
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
        } else if (auto val = extract_flag_value(arg, "--cache_policy="); !val.empty()) {
            if (val == "LFU") config.cache_policy = CachePolicy::LFU;
            else config.cache_policy = CachePolicy::LRU;
        }
    }
}

class KVStoreServiceImpl final : public KVStore::Service {
    private:
	    std::mutex mutex_; // thread safety
	    WAL wal_;
        DiskStore db_;
        CacheController cache_;
    public:
        Status PrintStats(ServerContext* context, const kvstore::Void* request, kvstore::Void* response) override {
            cache_.print_stats();  // your existing stats printer
            return Status::OK;
        }
        explicit KVStoreServiceImpl(std::string log_file, int cache_capacity, CachePolicy cache_policy) 
            : wal_(log_file), db_("rocksdb_data/"), cache_(cache_capacity, cache_policy) {
            recoverFromLog();
        }

        void recoverFromLog() {
            auto logs = wal_.recover();
            for (const auto& entry : logs) {
                if (entry.action == WALAction::PUT) {
                    db_.put(entry.key, entry.value);
                    cache_.put(entry.key, entry.value);
                } else {
                    db_.remove(entry.key);
                }
            }
            std::cout<<"[WAL] Recovery complete: "<<logs.size()<<" entries replayed.\n";
        }

	    Status Put(ServerContext* context, const PutRequest* request, PutReply* reply) override {
		    std::lock_guard<std::mutex> lock(mutex_);

			wal_.appendPut(request->key(), request->value());
            db_.put(request->key(), request->value());
            cache_.put(request->key(), request->value());
            
            std::cout<<"[Put] "<<request->key()<<" => "<<request->value()<<"\n";
			reply->set_success(true);
			return Status::OK;			
		}

		Status Get(ServerContext* context, const GetRequest* request, GetReply* reply) override {
	        std::lock_guard<std::mutex> lock(mutex_);
            //std::cout<<"[Get] "<<request->key()<<"||||||||||||||||\n";
            //cache_.print();
            auto val = cache_.get(request->key());
            if (val) {
                reply->set_found(true);
                reply->set_value(*val);
                std::cout<<"[CacheHit]"<<request->key()<<"\n";
            } else {
		        std::string dval;
                bool dget  = db_.get(request->key(), dval);
	            if (dget) { 
		            cache_.put(request->key(), dval);
                    reply->set_found(true);
			        reply->set_value(dval);
			        std::cout<<"[DbHit] "<<request->key()<<" => "<<dval<<"\n";
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
	        bool removed = true;
            removed &= db_.remove(request->key());
            cache_.remove(request->key());
            reply->set_success(removed);
	        std::cout<<"[Delete] "<<request->key()<<(removed ? " deleted" : " not found")<<"\n";
	        return Status::OK;
		}
};

int main(int argc, char** argv) {
    ServerConfig config;
    parse_flags(argc, argv, config);

    std::string server_address = "0.0.0.0:" + config.port;
    std::string cache_policy = "LRU";
    if (config.cache_policy == CachePolicy::LFU) cache_policy = "LFU";
    std::cout << "[Info] Starting server on " << server_address
              << " with cache_capacity=" << config.cache_capacity
              << " with cache_policy=" << cache_policy
              << " log_file=" << config.log_file << "\n";

    KVStoreServiceImpl service(config.log_file, config.cache_capacity, config.cache_policy);  // pass as needed

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());

    std::cout << "[Info] Server listening...\n";
    server->Wait();
    return 0;
}
