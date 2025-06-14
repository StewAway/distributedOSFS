CXX = g++
CXXFLAGS = -std=c++17 `pkg-config --cflags grpc++ grpc protobuf`
LDFLAGS = `pkg-config --libs grpc++ grpc protobuf`
PROTO_SRCS = filesystem.pb.cc filesystem.grpc.pb.cc

all: fs_server test_fs_client

fs_server: fs_server.cpp inode.o dir.o disk.o block_manager.o sfs.o $(PROTO_SRCS)
	$(CXX) $(CXXFLAGS) -o $@ fs_server.cpp inode.o dir.o disk.o block_manager.o sfs.o $(PROTO_SRCS) $(LDFLAGS)

test_fs_client: test_fs_client.cpp $(PROTO_SRCS)
	$(CXX) $(CXXFLAGS) -o $@ test_fs_client.cpp $(PROTO_SRCS) $(LDFLAGS)

inode.o: inode.cpp inode.h
	$(CXX) $(CXXFLAGS) -c inode.cpp

dir.o: dir.cpp dir.h
	$(CXX) $(CXXFLAGS) -c dir.cpp

disk.o: disk.cpp disk.h
	$(CXX) $(CXXFLAGS) -c disk.cpp

block_manager.o: block_manager.cpp block_manager.h
	$(CXX) $(CXXFLAGS) -c block_manager.cpp

sfs.o: sfs.cpp sfs.h
	$(CXX) $(CXXFLAGS) -c sfs.cpp

clean:
	rm -f fs_server test_fs_client *.o
