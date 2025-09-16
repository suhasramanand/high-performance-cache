#include "tcp_server.h"
#include "protocol.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <chrono>

namespace cache {

TCPServer::TCPServer(int port, size_t thread_pool_size)
    : port_(port), server_socket_(-1),
      thread_pool_(std::make_unique<ThreadPool>(thread_pool_size)),
      cache_(std::make_unique<Cache>()) {
}

TCPServer::~TCPServer() {
    stop();
}

bool TCPServer::start() {
    // Create socket
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return false;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Failed to set socket options" << std::endl;
        close(server_socket_);
        return false;
    }
    
    // Bind socket
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);
    
    if (bind(server_socket_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Failed to bind socket to port " << port_ << std::endl;
        close(server_socket_);
        return false;
    }
    
    // Listen for connections
    if (listen(server_socket_, 10) < 0) {
        std::cerr << "Failed to listen on socket" << std::endl;
        close(server_socket_);
        return false;
    }
    
    running_ = true;
    std::cout << "Cache server started on port " << port_ << std::endl;
    
    // Accept connections in a loop
    while (running_) {
        struct sockaddr_in client_address;
        socklen_t client_len = sizeof(client_address);
        
        int client_socket = accept(server_socket_, (struct sockaddr*)&client_address, &client_len);
        if (client_socket < 0) {
            if (running_) {
                std::cerr << "Failed to accept connection" << std::endl;
            }
            continue;
        }
        
        connections_handled_++;
        
        // Handle client in thread pool
        thread_pool_->enqueue([this, client_socket]() {
            handle_client(client_socket);
        });
    }
    
    return true;
}

void TCPServer::stop() {
    running_ = false;
    
    if (server_socket_ >= 0) {
        close(server_socket_);
        server_socket_ = -1;
    }
    
    thread_pool_->shutdown();
    std::cout << "Cache server stopped" << std::endl;
}

bool TCPServer::is_running() const {
    return running_.load();
}

void TCPServer::handle_client(int client_socket) {
    char buffer[4096];
    
    while (running_) {
        ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            break;
        }
        
        buffer[bytes_received] = '\0';
        std::string request(buffer);
        
        auto start_time = std::chrono::high_resolution_clock::now();
        std::string response = process_request(request);
        auto end_time = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        double current_total = total_response_time_.load();
        while (!total_response_time_.compare_exchange_weak(current_total, current_total + duration.count())) {
            // Retry on failure
        }
        requests_processed_++;
        
        send_response(client_socket, response);
    }
    
    close(client_socket);
}

std::string TCPServer::process_request(const std::string& request) {
    return parse_and_execute(request);
}

std::string TCPServer::parse_and_execute(const std::string& command) {
    auto req = Protocol::parse_request(command);
    
    if (!req.valid) {
        return Protocol::format_error("Invalid command");
    }
    
    switch (req.command) {
        case Protocol::Command::SET:
            if (cache_->set(req.key, req.value)) {
                return Protocol::format_success();
            } else {
                return Protocol::format_error("Failed to set value");
            }
            
        case Protocol::Command::GET: {
            std::string value = cache_->get(req.key);
            if (value.empty()) {
                return Protocol::format_error("NOT_FOUND");
            } else {
                return Protocol::format_success(value);
            }
        }
        
        case Protocol::Command::DELETE:
            if (cache_->remove(req.key)) {
                return Protocol::format_success();
            } else {
                return Protocol::format_error("NOT_FOUND");
            }
            
        case Protocol::Command::CLEAR:
            cache_->clear();
            return Protocol::format_success();
            
        case Protocol::Command::STATS: {
            std::ostringstream stats;
            stats << "size=" << cache_->size()
                  << " hits=" << cache_->hits()
                  << " misses=" << cache_->misses()
                  << " hit_ratio=" << cache_->hit_ratio()
                  << " memory_usage=" << cache_->memory_usage()
                  << " connections=" << connections_handled_
                  << " requests=" << requests_processed_
                  << " avg_response_time=" << average_response_time() << "μs";
            return Protocol::format_success(stats.str());
        }
        
        default:
            return Protocol::format_error("Unknown command");
    }
}

void TCPServer::send_response(int client_socket, const std::string& response) {
    std::string full_response = response + "\n";
    send(client_socket, full_response.c_str(), full_response.length(), 0);
}

size_t TCPServer::connections_handled() const {
    return connections_handled_.load();
}

size_t TCPServer::requests_processed() const {
    return requests_processed_.load();
}

double TCPServer::average_response_time() const {
    size_t requests = requests_processed_.load();
    if (requests == 0) return 0.0;
    return total_response_time_.load() / requests;
}

} // namespace cache
