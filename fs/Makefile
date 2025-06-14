CXX = g++
CXXFLAGS = -std=c++17 `pkg-config --cflags grpc++ grpc protobuf`
LDFLAGS = `pkg-config --libs grpc++ grpc protobuf`

PROTO = filesystem.proto
PROTO_OBJS = filesystem.pb.cc filesystem.grpc.pb.cc

SRCS = fs_server.cpp sfs.cpp disk.cpp inode.cpp dir.cpp block_manager.cpp $(PROTO_OBJS)
OBJS = $(SRCS:.cpp=.o)

all: fs_server test_fs_client

filesystem.pb.cc filesystem.grpc.pb.cc: $(PROTO)
	protoc -I=. --cpp_out=. --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` $<

fs_server: $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

test_fs_client: test_fs_client.cpp filesystem.pb.cc filesystem.grpc.pb.cc
	$(CXX) test_fs_client.cpp filesystem.pb.cc filesystem.grpc.pb.cc -o test_fs_client $(CXXFLAGS) $(LDFLAGS)

clean:
	rm -f *.o *.pb.* fs_server test_fs_client disk.img
