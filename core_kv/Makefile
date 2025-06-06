CXX = g++
CXXFLAGS = -std=c++17 `pkg-config --cflags grpc++ grpc protobuf`
LDFLAGS = `pkg-config --libs grpc++ grpc protobuf`
PROTO_SRCS = kvstore.pb.cc kvstore.grpc.pb.cc

all: kvstore_server kvstore_client

kvstore_server: kvstore_server.cpp $(PROTO_SRCS)
	$(CXX) $(CXXFLAGS) -o $@ kvstore_server.cpp $(PROTO_SRCS) $(LDFLAGS)

kvstore_client: kvstore_client.cpp $(PROTO_SRCS)
	$(CXX) $(CXXFLAGS) -o $@ kvstore_client.cpp $(PROTO_SRCS) $(LDFLAGS)

clean:
	rm -f kvstore_server kvstore_client *.o
