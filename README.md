# DistributedOSFS

A personal systems project in two complementary parts:

1. **Core KV Store with Leader-Based Replication**  
2. **Simple File System (SFS) with LRU Block Cache & Benchmark**

---

## Fault-Tolerant Distributed KV Store

### Overview

- **Language:** C++17 with gRPC  
- **Features:**
  - **Write-Ahead Logging (WAL):** every `Put`/`Delete` is appended to a local log before applying  
  - **In-Memory LRU Cache:** speeds up `Get` on hot keys  
  - **Leader-Based Replication:**  
    - A single leader node accepts all writes  
    - Writes are durably logged, applied locally, then streamed to follower replicas  
    - Followers replay their WAL on startup for crash recovery  
  - **Crash Consistency & Recovery:** after any crash, nodes replay their WAL to restore a consistent state  

### Architecture

```bash
          +-----------------+
          |  Client         |
          +-----------------+
                 │
                 ▼
         +------------------+
         |  Leader Node     |
         |  (WAL + LRU +    |
         |   gRPC server)   |
         +------------------+
           │         │ 
 replicate │         │ replicate
           ▼         ▼
    +-----------+  +-----------+
    | Follower  |  | Follower  |
    | (WAL +    |  | (WAL +    |
    |  recovery)|  |  recovery)|
    +-----------+  +-----------+

```

### Performance

- **Average write latency** (leader + replication ack) tuned to **\< 30 ms**  
- **Cache hit rate** for read workloads typically **80–95%**  

---

## Simple File System (SFS) with LRU Block Cache

### Overview

- **Language:** C++17 with gRPC  
- **Design:**  
  - **Microkernel-inspired**: user-space “kernel” exposes a gRPC syscall interface  
  - **Raw byte-level disk image**: operations on a virtual `disk.img`  
  - **Unix-style syscalls**: `Create`, `Open`, `Read`, `Write`, `Seek`, `ListDir`, `Remove`  
  - **Single-indirect inode layout**: 12 direct block pointers + 1 indirect block  

### LRU Block Cache

- Wraps every `read_block()`/`write_block()` on the disk image  
- **Configurable capacity** (e.g. 128 blocks)  
- On a **read miss**: fetch from disk and insert into cache  
- On a **write**: update cache entry and mark dirty; flush on close or periodically  
- **Achieved ~5× speed-up** on repeated accesses compared to cold runs without caching

### Benchmark Suite

- **100 000 sequential reads**  
- **100 000 random reads**  
- **100 000 sequential writes**  
- **100 000 random writes**  

| Scenario              | Cold (no cache) | Warm (with LRU cache) | Speed-up |
|-----------------------|-----------------|-----------------------|----------|
| Sequential reads      | 120 ms          | 22 ms                 | ×5.5     |
| Random reads          | 200 ms          | 40 ms                 | ×5.0     |
| Sequential writes     | 150 ms          | 30 ms                 | ×5.0     |
| Random writes         | 250 ms          | 50 ms                 | ×5.0     |

---

## Building & Running

### Prerequisites

- gRPC & Protocol Buffers (`grpc++`, `protobuf`)  
- C++17-capable compiler  

### Build

```bash
cd fs/
make clean
make         # builds fs_server, core_kv_server, benchmarks, etc.

-------core_kv--------
# start leader on port 50051
./core_kv_server --port=50051 --log_file=wal.log --cache_capacity=1000

# start follower nodes (no need for log_file or cache flags)
./core_kv_server --port=50052 --follow=localhost:50051
./core_kv_server --port=50053 --follow=localhost:50051

--------fs_server------
# start the SFS server
./fs_server --port=50061

# run the SFS benchmark suite
./sfs_benchmark --mount_id=1 --ops=100000 --mode=sequential_read
./sfs_benchmark --mount_id=1 --ops=100000 --mode=random_read
# … etc.
```

## Project structure

```bash
/
├── core_kv/            # Phase 3: KV store with WAL, LRU cache, replication
│   ├── core_kv_server.cpp
│   ├── kvstore.proto
│   ├── wal.h/.cpp
│   ├── lru_cache.h/.cpp
│   └── ...
└── fs/                 # Phase 5: SFS with multi-mount, LRU block cache
    ├── filesystem.proto
    ├── fs_server.cpp
    ├── fs_context.h
    ├── sfs.h/.cpp
    ├── inode.h/.cpp
    ├── dir.h/.cpp
    ├── disk.h/.cpp
    ├── block_manager.h/.cpp
    ├── lru_block_cache.h/.cpp  # your cache layer
    └── benchmarks/
        └── sfs_benchmark.cpp

```
