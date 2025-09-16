#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

class CacheClient {
public:
    CacheClient(const std::string& host, int port) : host_(host), port_(port), socket_(-1) {}
    
    ~CacheClient() {
        disconnect();
    }
    
    bool connect() {
        socket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_ < 0) {
            std::cerr << "Failed to create socket" << std::endl;
            return false;
        }
        
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port_);
        
        if (inet_pton(AF_INET, host_.c_str(), &server_addr.sin_addr) <= 0) {
            std::cerr << "Invalid address: " << host_ << std::endl;
            close(socket_);
            socket_ = -1;
            return false;
        }
        
        if (::connect(socket_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            std::cerr << "Failed to connect to " << host_ << ":" << port_ << std::endl;
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
        
        // Remove trailing newline
        if (!response.empty() && response.back() == '\n') {
            response.pop_back();
        }
        
        return response;
    }
    
    bool set(const std::string& key, const std::string& value) {
        std::string command = "SET " + key + " " + value;
        std::string response = send_command(command);
        return response.find("OK") == 0;
    }
    
    std::string get(const std::string& key) {
        std::string command = "GET " + key;
        std::string response = send_command(command);
        
        if (response.find("OK") == 0) {
            // Extract value from "OK value"
            size_t space_pos = response.find(' ');
            if (space_pos != std::string::npos) {
                return response.substr(space_pos + 1);
            }
        }
        
        return "";
    }
    
    bool remove(const std::string& key) {
        std::string command = "DELETE " + key;
        std::string response = send_command(command);
        return response.find("OK") == 0;
    }
    
    void clear() {
        send_command("CLEAR");
    }
    
    std::string stats() {
        return send_command("STATS");
    }

private:
    std::string host_;
    int port_;
    int socket_;
};

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options] [command]\n"
              << "Options:\n"
              << "  --host HOST    Server host (default: localhost)\n"
              << "  --port PORT    Server port (default: 8080)\n"
              << "  --help         Show this help message\n"
              << "\n"
              << "Commands:\n"
              << "  set KEY VALUE  Set a key-value pair\n"
              << "  get KEY        Get a value by key\n"
              << "  delete KEY     Delete a key\n"
              << "  clear          Clear all data\n"
              << "  stats          Show server statistics\n"
              << "  interactive    Start interactive mode\n";
}

int main(int argc, char* argv[]) {
    std::string host = "localhost";
    int port = 8080;
    std::string command;
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--host" && i + 1 < argc) {
            host = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else if (command.empty()) {
            command = arg;
        }
    }
    
    CacheClient client(host, port);
    
    if (!client.connect()) {
        std::cerr << "Failed to connect to cache server" << std::endl;
        return 1;
    }
    
    std::cout << "Connected to cache server at " << host << ":" << port << std::endl;
    
    if (command == "interactive" || command.empty()) {
        // Interactive mode
        std::string line;
        std::cout << "Enter commands (type 'quit' to exit):\n";
        
        while (std::getline(std::cin, line)) {
            if (line == "quit" || line == "exit") {
                break;
            }
            
            if (line.empty()) {
                continue;
            }
            
            std::string response = client.send_command(line);
            std::cout << response << std::endl;
        }
    } else {
        // Single command mode
        if (command == "stats") {
            std::cout << client.stats() << std::endl;
        } else if (command == "clear") {
            client.clear();
            std::cout << "Cache cleared" << std::endl;
        } else {
            std::cout << "Unknown command: " << command << std::endl;
            print_usage(argv[0]);
            return 1;
        }
    }
    
    return 0;
}
