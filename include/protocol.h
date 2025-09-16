#pragma once

#include <string>
#include <vector>

namespace cache {

class Protocol {
public:
    enum class Command {
        SET,
        GET,
        DELETE,
        CLEAR,
        STATS,
        UNKNOWN
    };

    struct Request {
        Command command;
        std::string key;
        std::string value;
        bool valid;
    };

    struct Response {
        bool success;
        std::string message;
        std::string data;
    };

    static Request parse_request(const std::string& request);
    static std::string format_response(const Response& response);
    static std::string format_error(const std::string& error);
    static std::string format_success(const std::string& data = "");

private:
    static std::vector<std::string> split(const std::string& str, char delimiter);
    static Command parse_command(const std::string& cmd);
};

} // namespace cache
