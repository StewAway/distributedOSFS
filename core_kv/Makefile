XX = g++
CXXFLAGS = -std=c++17 `pkg-config --cflags grpc++ grpc protobuf` -I/usr/local/include
LDFLAGS = `pkg-config --libs grpc++ grpc protobuf` -L/usr/local/lib -lrocksdb

PROTO_SRCS = kvstore.pb.cc kvstore.grpc.pb.cc

all: kvstore_server kvstore_client test_wal test_lru_cache benchmark_client

kvstore_server: kvstore_server.cpp wal.o disk_store.o $(PROTO_SRCS)
	$(XX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

kvstore_client: kvstore_client.cpp $(PROTO_SRCS)
	$(XX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

test_wal: test_wal.cpp wal.o
	$(XX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

test_lru_cache: test_lru_cache.cpp
	$(XX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

benchmark_client: test_benchmark_client.cpp $(PROTO_SRCS)
	$(XX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

wal.o: wal.cpp wal.h
	$(XX) $(CXXFLAGS) -c $<

disk_store.o: disk_store.cpp disk_store.h
	$(XX) $(CXXFLAGS) -c $<

clean:
	rm -f kvstore_server kvstore_client test_wal test_lru_cache benchmark_client *.o *.log
