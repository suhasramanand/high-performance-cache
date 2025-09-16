#include "protocol.h"
#include <sstream>
#include <algorithm>

namespace cache {

Protocol::Request Protocol::parse_request(const std::string& request) {
    Request req;
    req.valid = false;
    
    auto parts = split(request, ' ');
    if (parts.empty()) {
        return req;
    }
    
    req.command = parse_command(parts[0]);
    
    switch (req.command) {
        case Command::SET:
            if (parts.size() >= 3) {
                req.key = parts[1];
                // Join remaining parts as value (in case value contains spaces)
                std::ostringstream value_stream;
                for (size_t i = 2; i < parts.size(); ++i) {
                    if (i > 2) value_stream << " ";
                    value_stream << parts[i];
                }
                req.value = value_stream.str();
                req.valid = true;
            }
            break;
            
        case Command::GET:
        case Command::DELETE:
            if (parts.size() >= 2) {
                req.key = parts[1];
                req.valid = true;
            }
            break;
            
        case Command::CLEAR:
        case Command::STATS:
            req.valid = true;
            break;
            
        default:
            req.valid = false;
            break;
    }
    
    return req;
}

std::string Protocol::format_response(const Response& response) {
    std::ostringstream oss;
    
    if (response.success) {
        oss << "OK";
        if (!response.data.empty()) {
            oss << " " << response.data;
        }
        if (!response.message.empty()) {
            oss << " " << response.message;
        }
    } else {
        oss << "ERROR " << response.message;
    }
    
    return oss.str();
}

std::string Protocol::format_error(const std::string& error) {
    return "ERROR " + error;
}

std::string Protocol::format_success(const std::string& data) {
    if (data.empty()) {
        return "OK";
    }
    return "OK " + data;
}

std::vector<std::string> Protocol::split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    
    return tokens;
}

Protocol::Command Protocol::parse_command(const std::string& cmd) {
    std::string upper_cmd = cmd;
    std::transform(upper_cmd.begin(), upper_cmd.end(), upper_cmd.begin(), ::toupper);
    
    if (upper_cmd == "SET") return Command::SET;
    if (upper_cmd == "GET") return Command::GET;
    if (upper_cmd == "DELETE") return Command::DELETE;
    if (upper_cmd == "CLEAR") return Command::CLEAR;
    if (upper_cmd == "STATS") return Command::STATS;
    
    return Command::UNKNOWN;
}

} // namespace cache
