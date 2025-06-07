CXX = g++
CXXFLAGS = -std=c++17 `pkg-config --cflags grpc++ grpc protobuf`
LDFLAGS = `pkg-config --libs grpc++ grpc protobuf`
PROTO_SRCS = kvstore.pb.cc kvstore.grpc.pb.cc

all: kvstore_server kvstore_client test_wal test_lru_cache

kvstore_server: kvstore_server.cpp wal.o $(PROTO_SRCS)
	$(CXX) $(CXXFLAGS) -o $@ kvstore_server.cpp wal.o $(PROTO_SRCS) $(LDFLAGS)

kvstore_client: kvstore_client.cpp $(PROTO_SRCS)
	$(CXX) $(CXXFLAGS) -o $@ kvstore_client.cpp $(PROTO_SRCS) $(LDFLAGS)

test_wal: test_wal.cpp wal.o
	$(CXX) $(CXXFLAGS) -o $@ test_wal.cpp wal.o

test_lru_cache: test_lru_cache.cpp
	$(CXX) $(CXXFLAGS) -o $@ test_lru_cache.cpp

wal.o: wal.cpp wal.h
	$(CXX) $(CXXFLAGS) -c wal.cpp

clean:
	rm -f kvstore_server kvstore_client test_wal test_lru_cache *.o *.log
