PROTO_SRCS = kernel.pb.cc kernel.grpc.pb.cc
CXXFLAGS = -std=c++17 `pkg-config --cflags grpc++ protobuf`
LDFLAGS = `pkg-config --libs grpc++ protobuf`

all: kernel_server test_kernel_client

kernel_server: kernel_server.cpp syscall_handler.cpp $(PROTO_SRCS)
	$(CXX) $(CXXFLAGS) -o kernel_server $^ $(LDFLAGS)

test_kernel_client: test_kernel_client.cpp $(PROTO_SRCS)
	$(CXX) $(CXXFLAGS) -o test_kernel_client $^ $(LDFLAGS)

clean:
	rm -f kernel_server test_kernel_client *.o

