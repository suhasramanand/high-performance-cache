# High-Performance C++ Caching System

A high-performance, thread-safe, in-memory distributed cache implementation in C++, inspired by Memcached. This project demonstrates advanced C++ techniques including custom memory allocators, object pooling, LRU eviction policies, and multi-threaded TCP server architecture.

## Features

### Core Features
- **In-memory key-value storage** with string keys and values
- **LRU (Least Recently Used) eviction policy** when capacity is exceeded
- **Thread-safe access** using shared mutexes and atomic operations
- **Custom memory allocator** with object pooling for optimal performance
- **Multi-threaded request handling** with configurable thread pool

### Concurrency
- **Thread pool architecture** for handling multiple concurrent requests
- **Lock-free statistics** using atomic operations
- **Shared mutex** for read-heavy workloads (multiple readers, single writer)
- **Demonstrates throughput improvements** compared to single-threaded baseline

### Interface
- **Simple TCP server** accepting GET and SET requests
- **Protocol format**:
  - `SET key value` - Store a key-value pair
  - `GET key` - Retrieve a value by key
  - `DELETE key` - Remove a key
  - `CLEAR` - Clear all data
  - `STATS` - Show server statistics

### Benchmarking
- **Comprehensive benchmarking tool** supporting millions of requests
- **Measures latency, throughput, and cache hit ratio**
- **Multi-threaded client** for stress testing
- **Configurable read/write ratios** and thread counts

## Project Structure

```
high-performance-cache/
├── CMakeLists.txt          # CMake build configuration
├── README.md               # This file
├── include/                # Header files
│   ├── cache.h             # Main cache interface
│   ├── memory_allocator.h  # Custom memory allocator
│   ├── object_pool.h       # Object pooling template
│   ├── lru_cache.h         # LRU cache implementation
│   ├── thread_pool.h       # Thread pool implementation
│   ├── tcp_server.h        # TCP server interface
│   └── protocol.h          # Protocol parsing
├── src/                    # Source files
│   ├── cache.cpp           # Cache implementation
│   ├── memory_allocator.cpp # Memory allocator implementation
│   ├── object_pool.cpp     # Object pool implementation
│   ├── lru_cache.cpp       # LRU cache implementation
│   ├── thread_pool.cpp     # Thread pool implementation
│   ├── tcp_server.cpp      # TCP server implementation
│   ├── protocol.cpp        # Protocol implementation
│   ├── main.cpp            # Server main function
│   ├── client.cpp          # Client tool
│   └── benchmark.cpp       # Benchmarking tool
└── tests/                  # Unit tests
    ├── test_cache.cpp      # Cache tests
    ├── test_memory_allocator.cpp # Memory allocator tests
    └── test_lru_cache.cpp  # LRU cache tests
```

## Building

### Prerequisites
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.16+
- Make or Ninja build system

### Build Steps

1. **Clone and navigate to the project:**
   ```bash
   git clone <repository-url>
   cd high-performance-cache
   ```

2. **Create build directory:**
   ```bash
   mkdir build && cd build
   ```

3. **Configure with CMake:**
   ```bash
   cmake ..
   ```

4. **Build the project:**
   ```bash
   make -j$(nproc)
   ```

   Or with Ninja:
   ```bash
   ninja
   ```

### Build Options

- **Debug build:** `cmake -DCMAKE_BUILD_TYPE=Debug ..`
- **Release build:** `cmake -DCMAKE_BUILD_TYPE=Release ..`
- **Custom compiler flags:** `cmake -DCMAKE_CXX_FLAGS="-O3 -march=native" ..`

## Running

### Start the Cache Server

```bash
# Default configuration (port 8080, auto-detect CPU cores)
./cache_server

# Custom configuration
./cache_server --port 9090 --threads 8
```

**Server Options:**
- `--port PORT`: Server port (default: 8080)
- `--threads N`: Number of worker threads (default: CPU cores)
- `--help`: Show help message

### Using the Client Tool

```bash
# Interactive mode
./cache_client

# Single commands
./cache_client --host localhost --port 8080 set mykey myvalue
./cache_client --host localhost --port 8080 get mykey
./cache_client --host localhost --port 8080 stats
```

**Client Options:**
- `--host HOST`: Server host (default: localhost)
- `--port PORT`: Server port (default: 8080)
- `--help`: Show help message

### Running Benchmarks

```bash
# Basic benchmark (100K operations, 4 threads, 80% reads)
./cache_benchmark

# Custom benchmark
./cache_benchmark --operations 1000000 --threads 8 --read-ratio 0.9

# High-throughput test
./cache_benchmark --operations 10000000 --threads 16 --read-ratio 0.95
```

**Benchmark Options:**
- `--host HOST`: Server host (default: localhost)
- `--port PORT`: Server port (default: 8080)
- `--operations N`: Operations per thread (default: 100000)
- `--threads N`: Number of client threads (default: 4)
- `--read-ratio R`: Ratio of read operations (default: 0.8)
- `--no-warmup`: Skip warmup phase
- `--help`: Show help message

## Testing

### Run Unit Tests

```bash
# Run all tests
./cache_tests

# Run with verbose output
./cache_tests --gtest_verbose

# Run specific test suite
./cache_tests --gtest_filter="CacheTest.*"
```

### Test Coverage

The test suite includes:
- **Cache functionality tests**: Basic operations, concurrency, LRU eviction
- **Memory allocator tests**: Allocation, deallocation, fragmentation, concurrency
- **LRU cache tests**: Eviction policies, capacity management, access patterns

## Performance Characteristics

### Expected Performance
- **Throughput**: 100K+ operations/second on modern hardware
- **Latency**: Sub-millisecond average response time
- **Memory efficiency**: Custom allocator reduces fragmentation
- **Scalability**: Linear scaling with thread count (up to CPU cores)

### Benchmark Results Example
```
Benchmark Results
=================
Total operations: 1000000
Total errors: 0
Total time: 2345.67 ms
Throughput: 426,234 ops/sec
Average latency: 0.0023 ms
Min latency: 0.0001 ms
Max latency: 0.0156 ms
Error rate: 0.0%
```

## Architecture Details

### Memory Management
- **Custom allocator**: Reduces malloc/free overhead and fragmentation
- **Object pooling**: Reuses cache entry objects to minimize allocations
- **Memory alignment**: 8-byte aligned allocations for optimal performance

### Concurrency Model
- **Shared mutex**: Allows multiple concurrent readers
- **Thread pool**: Pre-allocated worker threads for request handling
- **Lock-free statistics**: Atomic counters for hit/miss tracking

### LRU Implementation
- **Doubly-linked list**: O(1) access time for LRU operations
- **Hash map**: O(1) key lookup with iterator tracking
- **Automatic eviction**: Removes least recently used entries when capacity exceeded

## Protocol Reference

### Commands

| Command | Format | Description | Response |
|---------|--------|-------------|----------|
| SET | `SET key value` | Store key-value pair | `OK` or `ERROR message` |
| GET | `GET key` | Retrieve value | `OK value` or `ERROR NOT_FOUND` |
| DELETE | `DELETE key` | Remove key | `OK` or `ERROR NOT_FOUND` |
| CLEAR | `CLEAR` | Clear all data | `OK` |
| STATS | `STATS` | Show statistics | `OK stats_string` |

### Example Session

```bash
$ telnet localhost 8080
Trying 127.0.0.1...
Connected to localhost.
Escape character is '^]'.
SET user:123 "John Doe"
OK
GET user:123
OK John Doe
SET user:456 "Jane Smith"
OK
STATS
OK size=2 hits=1 misses=0 hit_ratio=1 memory_usage=1024 connections=1 requests=3 avg_response_time=0.5μs
DELETE user:123
OK
GET user:123
ERROR NOT_FOUND
```

## Development

### Code Style
- **C++17 standard** with modern features
- **RAII principles** for resource management
- **Exception safety** with proper cleanup
- **Const correctness** throughout the codebase

### Contributing
1. Fork the repository
2. Create a feature branch
3. Add tests for new functionality
4. Ensure all tests pass
5. Submit a pull request

### Future Enhancements
- **Persistence**: Optional disk-based storage
- **Clustering**: Multi-node distributed cache
- **Compression**: Value compression for memory efficiency
- **Metrics**: Prometheus/Graphite integration
- **Configuration**: YAML/JSON configuration files

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- Inspired by Memcached and Redis architectures
- Uses Google Test framework for unit testing
- Built with modern C++ best practices
