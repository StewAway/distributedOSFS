#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <chrono>
#include <thread>

#include <grpcpp/grpcpp.h>
#include "kvstore.grpc.pb.h"
#include "replication.grpc.pb.h"

#include "wal.h"
#include "disk_store.h"
#include "cache_controller.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ClientContext;
using grpc::Status;
using kvstore::KVStore;
using kvstore::PutRequest;
using kvstore::PutReply;
using kvstore::GetRequest;
using kvstore::GetReply;
using kvstore::DeleteRequest;
using kvstore::DeleteReply;
using kvstore::Void;

using replication::Replication;
using replication::ReplicatePutRequest;
using replication::ReplicatePutReply;
using replication::HeartbeatRequest;
using replication::HeartbeatReply;

struct ServerConfig {
    int cache_capacity = 1000;
    std::string port = "50051";
    std::string log_file;
    CachePolicy cache_policy = CachePolicy::LRU;
    std::string node_id = "0";
    bool is_leader = false;
    std::vector<std::string> follower_addresses;
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
        } else if (auto val = extract_flag_value(arg, "--node_id="); !val.empty()) {
            config.node_id = val;
        } else if (auto val = extract_flag_value(arg, "--is_leader="); !val.empty()) {
            config.is_leader = (val == "true");
        } else if (auto val = extract_flag_value(arg, "--followers="); !val.empty()) {
            size_t start = 0, end = 0;
            while ((end = val.find(',', start)) != std::string::npos) {
                config.follower_addresses.push_back(val.substr(start, end - start));
                start = end + 1;
            }
            if (start < val.size()) {
                config.follower_addresses.push_back(val.substr(start));
            }
        }
    }
    config.log_file = "wal" + config.node_id + ".log";
}

class KVStoreServiceImpl final : public KVStore::Service, public Replication::Service {
    private:
	    std::mutex mutex_; // thread safety
	    WAL wal_;
        DiskStore db_;
        CacheController cache_;
        bool is_leader_;
        std::string node_id_;
        std::vector<std::unique_ptr<replication::Replication::Stub>> follower_stubs_;
    public:
        Status PrintStats(ServerContext* context, const kvstore::Void* request, kvstore::Void* response) override {
            cache_.print_stats();  // stats printer
            return Status::OK;
        }

        explicit KVStoreServiceImpl(const ServerConfig& config)
            : wal_(config.log_file), db_("rocksdb_data/" + config.node_id),
              cache_(config.cache_capacity, config.cache_policy),
              is_leader_(config.is_leader), node_id_(config.node_id) {
            for (const std::string& addr : config.follower_addresses) {
                follower_stubs_.emplace_back(replication::Replication::NewStub(
                    grpc::CreateChannel(addr, grpc::InsecureChannelCredentials())));
            }
            recoverFromLog();
        }

        void start_heartbeat_loop() {
            if (!is_leader_) return;
            std::thread([this]() {
                while (true) {
                    for (auto& stub : follower_stubs_) {
                        HeartbeatRequest req;
                        req.set_leader_id(node_id_);
                        HeartbeatReply reply;
                        ClientContext ctx;
                        Status status = stub->Heartbeat(&ctx, req, &reply);
                        if (!status.ok()) {
                            std::cerr<<"[Heartbeat] Failed: "<<status.error_message()<<std::endl;
                        } else {
                            std::cerr<<"[Heartbeat] ACK from follower "<<reply.follower_id()<<std::endl;
                        }
                    }
                    std::this_thread::sleep_for(std::chrono::seconds(3));
                }
            }).detach();
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
            
            if (is_leader_) {
                for (auto& stub : follower_stubs_) {
                    ReplicatePutRequest rreq;
                    rreq.set_key(request->key());
                    rreq.set_value(request->value());
                    ReplicatePutReply rrep;
                    ClientContext ctx;
                    Status s = stub->ReplicatePut(&ctx, rreq, &rrep);
                    if (!s.ok()) {
                        std::cerr<<"[Warning] Failed to replicate to follower: "<<s.error_message()<<"\n";
                    }
                }
            }

            std::cout<<"[Put] "<<request->key()<<" => "<<request->value()<<"\n";
			reply->set_success(true);
			return Status::OK;			
		}

        Status ReplicatePut(ServerContext * context, const ReplicatePutRequest* req, ReplicatePutReply* rep) override {
            std::lock_guard<std::mutex> lock(mutex_);
            wal_.appendPut(req->key(), req->value());
            db_.put(req->key(), req->value());
            cache_.put(req->key(), req->value());
            rep->set_success(true);
            std::cout<<"[Replicated] "<<req->key()<<" => "<<req->value()<<"\n";
            return Status::OK;
        }

        Status Heartbeat(ServerContext* context, const HeartbeatRequest* req, HeartbeatReply* res) override {
            res->set_alive(true);
            res->set_follower_id(node_id_);
            std::cout<<"[Heartbeat] Received from leader "<<req->leader_id()<<"\n";
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
	        bool removed = db_.remove(request->key());
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
              << " log_file=" << config.log_file
              << " node_id=" << config.node_id
              << (config.is_leader ? " [LEADER]" : "[FOLLOWER]") <<"\n";

    KVStoreServiceImpl service(config);  // pass as needed
    service.start_heartbeat_loop();
    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(static_cast<kvstore::KVStore::Service*>(&service));
    builder.RegisterService(static_cast<replication::Replication::Service*>(&service));
    std::unique_ptr<Server> server(builder.BuildAndStart());

    std::cout << "[Info] Server listening...\n";
    server->Wait();
    return 0;
}
