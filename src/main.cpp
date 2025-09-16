#include "tcp_server.h"
#include <iostream>
#include <signal.h>
#include <unistd.h>

namespace {
    std::unique_ptr<cache::TCPServer> g_server;
    
    void signal_handler(int signal) {
        std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
        if (g_server) {
            g_server->stop();
        }
    }
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    int port = 8080;
    size_t thread_pool_size = std::thread::hardware_concurrency();
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "--threads" && i + 1 < argc) {
            thread_pool_size = std::stoul(argv[++i]);
        } else if (arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options]\n"
                      << "Options:\n"
                      << "  --port PORT      Server port (default: 8080)\n"
                      << "  --threads N      Number of worker threads (default: CPU cores)\n"
                      << "  --help           Show this help message\n";
            return 0;
        }
    }
    
    // Set up signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    std::cout << "Starting High-Performance Cache Server..." << std::endl;
    std::cout << "Port: " << port << std::endl;
    std::cout << "Thread pool size: " << thread_pool_size << std::endl;
    
    // Create and start server
    g_server = std::make_unique<cache::TCPServer>(port, thread_pool_size);
    
    if (!g_server->start()) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }
    
    return 0;
}
