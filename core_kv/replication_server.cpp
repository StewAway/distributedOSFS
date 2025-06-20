#include "cluster_context.h"
#include "replication.grpc.pb.h"
#include <grpcpp/grpcpp.h>

class ReplicationServiceImpl final : public Replication::Service {
public:
    grpc::Status AppendEntries(grpc::ServerContext* ctx, const AppendRequest* req, AppendReply* rep) override {
        for (const auto& entry 
    }
}
