CXX = g++
CXXFLAGS = -g -std=c++17 -lstdc++fs `pkg-config --cflags grpc++ grpc protobuf`
LDFLAGS = `pkg-config --libs grpc++ grpc protobuf`
PROTO_SRCS = filesystem.pb.cc filesystem.grpc.pb.cc

SRCS = inode.cpp dir.cpp disk.cpp block_manager.cpp sfs.cpp block_cache_controller.cpp
OBJS = $(SRCS:.cpp=.o)

all: fs_server test_correctness_fs_client test_benchmark_fs_client

fs_server: fs_server.cpp $(OBJS) $(PROTO_SRCS)
	$(CXX) $(CXXFLAGS) -o $@ fs_server.cpp $(OBJS) $(PROTO_SRCS) $(LDFLAGS)

test_correctness_fs_client: test_correctness_fs_client.cpp $(PROTO_SRCS)
	$(CXX) $(CXXFLAGS) -o $@ test_correctness_fs_client.cpp $(PROTO_SRCS) $(LDFLAGS)

test_benchmark_fs_client: test_benchmark_fs_client.cpp $(PROTO_SRCS)
	$(CXX) $(CXXFLAGS) -o $@ test_benchmark_fs_client.cpp $(PROTO_SRCS) $(LDFLAGS)

%.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -f fs_server test_*_fs_client *.o *.img

clean-images:
	rm -f images/*.img
