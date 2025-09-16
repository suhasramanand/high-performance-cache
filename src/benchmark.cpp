#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <random>
#include <atomic>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

class BenchmarkClient {
public:
    BenchmarkClient(const std::string& host, int port) : host_(host), port_(port) {}
    
    struct Result {
        size_t operations = 0;
        size_t errors = 0;
        double total_time_ms = 0.0;
        double min_latency_ms = std::numeric_limits<double>::max();
        double max_latency_ms = 0.0;
        double total_latency_ms = 0.0;
    };
    
    Result run_benchmark(size_t num_operations, double read_ratio = 0.8) {
        Result result;
        
        if (!connect()) {
            result.errors = num_operations;
            return result;
        }
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.0, 1.0);
        std::uniform_int_distribution<> key_dis(1, 1000000);
        std::uniform_int_distribution<> value_dis(10, 1000);
        
        for (size_t i = 0; i < num_operations; ++i) {
            auto op_start = std::chrono::high_resolution_clock::now();
            
            std::string key = "key_" + std::to_string(key_dis(gen));
            
            if (dis(gen) < read_ratio) {
                // Read operation
                std::string response = send_command("GET " + key);
                if (response.find("ERROR") != 0) {
                    result.operations++;
                } else {
                    result.errors++;
                }
            } else {
                // Write operation
                std::string value = "value_" + std::to_string(value_dis(gen));
                std::string response = send_command("SET " + key + " " + value);
                if (response.find("OK") == 0) {
                    result.operations++;
                } else {
                    result.errors++;
                }
            }
            
            auto op_end = std::chrono::high_resolution_clock::now();
            double latency_ms = std::chrono::duration<double, std::milli>(op_end - op_start).count();
            
            result.min_latency_ms = std::min(result.min_latency_ms, latency_ms);
            result.max_latency_ms = std::max(result.max_latency_ms, latency_ms);
            result.total_latency_ms += latency_ms;
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        result.total_time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        
        disconnect();
        return result;
    }

private:
    std::string host_;
    int port_;
    int socket_ = -1;
    
    bool connect() {
        socket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_ < 0) {
            return false;
        }
        
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port_);
        
        if (inet_pton(AF_INET, host_.c_str(), &server_addr.sin_addr) <= 0) {
            close(socket_);
            socket_ = -1;
            return false;
        }
        
        if (::connect(socket_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            close(socket_);
            socket_ = -1;
            return false;
        }
        
        return true;
    }
    
    void disconnect() {
        if (socket_ >= 0) {
            close(socket_);
            socket_ = -1;
        }
    }
    
    std::string send_command(const std::string& command) {
        if (socket_ < 0) {
            return "ERROR Not connected";
        }
        
        std::string full_command = command + "\n";
        if (send(socket_, full_command.c_str(), full_command.length(), 0) < 0) {
            return "ERROR Send failed";
        }
        
        char buffer[4096];
        ssize_t bytes_received = recv(socket_, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            return "ERROR Receive failed";
        }
        
        buffer[bytes_received] = '\0';
        std::string response(buffer);
        
        if (!response.empty() && response.back() == '\n') {
            response.pop_back();
        }
        
        return response;
    }
};

struct BenchmarkConfig {
    std::string host = "localhost";
    int port = 8080;
    size_t num_operations = 100000;
    size_t num_threads = 4;
    double read_ratio = 0.8;
    bool warmup = true;
};

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options]\n"
              << "Options:\n"
              << "  --host HOST        Server host (default: localhost)\n"
              << "  --port PORT        Server port (default: 8080)\n"
              << "  --operations N     Number of operations per thread (default: 100000)\n"
              << "  --threads N        Number of client threads (default: 4)\n"
              << "  --read-ratio R     Ratio of read operations (default: 0.8)\n"
              << "  --no-warmup        Skip warmup phase\n"
              << "  --help             Show this help message\n";
}

BenchmarkConfig parse_arguments(int argc, char* argv[]) {
    BenchmarkConfig config;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--host" && i + 1 < argc) {
            config.host = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            config.port = std::stoi(argv[++i]);
        } else if (arg == "--operations" && i + 1 < argc) {
            config.num_operations = std::stoul(argv[++i]);
        } else if (arg == "--threads" && i + 1 < argc) {
            config.num_threads = std::stoul(argv[++i]);
        } else if (arg == "--read-ratio" && i + 1 < argc) {
            config.read_ratio = std::stod(argv[++i]);
        } else if (arg == "--no-warmup") {
            config.warmup = false;
        } else if (arg == "--help") {
            print_usage(argv[0]);
            exit(0);
        }
    }
    
    return config;
}

void warmup_cache(const BenchmarkConfig& config) {
    std::cout << "Warming up cache..." << std::endl;
    
    BenchmarkClient client(config.host, config.port);
    auto result = client.run_benchmark(10000, 0.5); // 50% read/write ratio for warmup
    
    std::cout << "Warmup completed: " << result.operations << " operations in " 
              << result.total_time_ms << " ms" << std::endl;
}

int main(int argc, char* argv[]) {
    auto config = parse_arguments(argc, argv);
    
    std::cout << "High-Performance Cache Benchmark" << std::endl;
    std::cout << "=================================" << std::endl;
    std::cout << "Host: " << config.host << std::endl;
    std::cout << "Port: " << config.port << std::endl;
    std::cout << "Operations per thread: " << config.num_operations << std::endl;
    std::cout << "Number of threads: " << config.num_threads << std::endl;
    std::cout << "Read ratio: " << config.read_ratio << std::endl;
    std::cout << std::endl;
    
    // Warmup phase
    if (config.warmup) {
        warmup_cache(config);
    }
    
    // Benchmark phase
    std::cout << "Starting benchmark..." << std::endl;
    
    std::vector<std::thread> threads;
    std::vector<BenchmarkClient::Result> results(config.num_threads);
    std::atomic<size_t> completed_threads{0};
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < config.num_threads; ++i) {
        threads.emplace_back([&, i]() {
            BenchmarkClient client(config.host, config.port);
            results[i] = client.run_benchmark(config.num_operations, config.read_ratio);
            completed_threads++;
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    double total_time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    
    // Aggregate results
    BenchmarkClient::Result total_result;
    for (const auto& result : results) {
        total_result.operations += result.operations;
        total_result.errors += result.errors;
        total_result.min_latency_ms = std::min(total_result.min_latency_ms, result.min_latency_ms);
        total_result.max_latency_ms = std::max(total_result.max_latency_ms, result.max_latency_ms);
        total_result.total_latency_ms += result.total_latency_ms;
    }
    
    // Print results
    std::cout << "\nBenchmark Results" << std::endl;
    std::cout << "=================" << std::endl;
    std::cout << "Total operations: " << total_result.operations << std::endl;
    std::cout << "Total errors: " << total_result.errors << std::endl;
    std::cout << "Total time: " << total_time_ms << " ms" << std::endl;
    std::cout << "Throughput: " << (total_result.operations / (total_time_ms / 1000.0)) << " ops/sec" << std::endl;
    std::cout << "Average latency: " << (total_result.total_latency_ms / total_result.operations) << " ms" << std::endl;
    std::cout << "Min latency: " << total_result.min_latency_ms << " ms" << std::endl;
    std::cout << "Max latency: " << total_result.max_latency_ms << " ms" << std::endl;
    std::cout << "Error rate: " << (100.0 * total_result.errors / (total_result.operations + total_result.errors)) << "%" << std::endl;
    
    return 0;
}
