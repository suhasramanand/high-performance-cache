#pragma once

#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "thread_pool.h"
#include "cache.h"

namespace cache {

class TCPServer {
public:
    explicit TCPServer(int port = 8080, size_t thread_pool_size = 4);
    ~TCPServer();

    // Non-copyable, non-movable
    TCPServer(const TCPServer&) = delete;
    TCPServer& operator=(const TCPServer&) = delete;
    TCPServer(TCPServer&&) = delete;
    TCPServer& operator=(TCPServer&&) = delete;

    bool start();
    void stop();
    bool is_running() const;

    // Statistics
    size_t connections_handled() const;
    size_t requests_processed() const;
    double average_response_time() const;

private:
    int port_;
    int server_socket_;
    std::atomic<bool> running_{false};
    std::unique_ptr<ThreadPool> thread_pool_;
    std::unique_ptr<Cache> cache_;
    
    // Statistics
    std::atomic<size_t> connections_handled_{0};
    std::atomic<size_t> requests_processed_{0};
    std::atomic<double> total_response_time_{0.0};
    
    void handle_client(int client_socket);
    std::string process_request(const std::string& request);
    std::string parse_and_execute(const std::string& command);
    void send_response(int client_socket, const std::string& response);
};

} // namespace cache
