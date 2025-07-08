/**
 * @file JsonRpcProtocol.cpp
 * @brief JSON-RPC 2.0 protocol implementation for LSP communication
 */

#include "als/core/JsonRpcProtocol.h"
#include <iostream>
#include <sstream>
#include <algorithm>

namespace als {
namespace core {

JsonRpcProtocol::JsonRpcProtocol(std::istream& input, std::ostream& output)
    : input_stream_(input)
    , output_stream_(output)
    , connected_(true) {
}

std::optional<JsonRpcMessage> JsonRpcProtocol::readMessage() {
    if (!connected_) {
        return std::nullopt;
    }

    try {
        // Read Content-Length header
        auto content_length = readContentLengthHeader();
        if (!content_length.has_value()) {
            connected_ = false;
            return std::nullopt;
        }

        size_t length = std::stoul(content_length.value());
        if (length == 0 || length > 100 * 1024 * 1024) { // 100MB limit
            std::cerr << "[JsonRpcProtocol] Invalid content length: " << length << std::endl;
            return std::nullopt;
        }

        // Read JSON payload
        std::string json_content = readJsonPayload(length);
        if (json_content.empty()) {
            connected_ = false;
            return std::nullopt;
        }

        // Parse and return message
        return parseMessage(json_content);

    } catch (const std::exception& e) {
        std::cerr << "[JsonRpcProtocol] Error reading message: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::optional<JsonRpcMessage> JsonRpcProtocol::parseMessage(const std::string& json_content) {
    try {
        auto json = nlohmann::json::parse(json_content);

        // Validate basic JSON-RPC structure
        if (!validateJsonRpcMessage(json)) {
            std::cerr << "[JsonRpcProtocol] Invalid JSON-RPC message format" << std::endl;
            return std::nullopt;
        }

        // Determine message type and create appropriate object
        JsonRpcMessageType type = determineMessageType(json);

        switch (type) {
            case JsonRpcMessageType::Request:
                return JsonRpcRequest(json);
            case JsonRpcMessageType::Notification:
                return JsonRpcNotification(json);
            case JsonRpcMessageType::Response:
            case JsonRpcMessageType::Error:
                // For now, we'll handle responses as generic messages
                return JsonRpcMessage(type, json);
        }

        return std::nullopt;

    } catch (const nlohmann::json::exception& e) {
        std::cerr << "[JsonRpcProtocol] JSON parsing error: " << e.what() << std::endl;
        writeParseError("Parse error: " + std::string(e.what()));
        return std::nullopt;
    }
}

void JsonRpcProtocol::writeMessage(const JsonRpcResponse& response) {
    writeRawMessage(response.raw.dump());
}

void JsonRpcProtocol::writeMessage(const JsonRpcNotification& notification) {
    nlohmann::json json = {
        {"jsonrpc", "2.0"},
        {"method", notification.method}
    };

    if (!notification.params.is_null()) {
        json["params"] = notification.params;
    }

    writeRawMessage(json.dump());
}

void JsonRpcProtocol::writeMessage(const JsonRpcError& error) {
    writeRawMessage(error.raw.dump());
}

void JsonRpcProtocol::writeError(const JsonRpcId& id, int code, const std::string& message,
                                const nlohmann::json& data) {
    JsonRpcError error(id, code, message, data);
    writeMessage(error);
}

void JsonRpcProtocol::writeParseError(const std::string& message) {
    writeError(nullptr, -32700, "Parse error: " + message);
}

bool JsonRpcProtocol::isConnected() const {
    return connected_;
}

void JsonRpcProtocol::disconnect() {
    connected_ = false;
}

std::optional<std::string> JsonRpcProtocol::readContentLengthHeader() {
    std::string line;

    // Read headers until we find Content-Length or empty line
    while (std::getline(input_stream_, line)) {
        // Remove \r if present (Windows line endings)
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        // Empty line indicates end of headers
        if (line.empty()) {
            break;
        }

        // Look for Content-Length header
        if (line.find("Content-Length:") == 0) {
            std::string length_str = line.substr(15); // Skip "Content-Length:"

            // Trim whitespace
            size_t start = length_str.find_first_not_of(" \t");
            if (start != std::string::npos) {
                length_str = length_str.substr(start);
                size_t end = length_str.find_last_not_of(" \t\r\n");
                if (end != std::string::npos) {
                    length_str = length_str.substr(0, end + 1);
                }
                return length_str;
            }
        }
    }

    // Check if we hit EOF
    if (input_stream_.eof()) {
        return std::nullopt;
    }

    // No Content-Length header found
    std::cerr << "[JsonRpcProtocol] No Content-Length header found" << std::endl;
    return std::nullopt;
}

std::string JsonRpcProtocol::readJsonPayload(size_t content_length) {
    std::string payload(content_length, '\0');
    input_stream_.read(&payload[0], content_length);

    if (input_stream_.gcount() != static_cast<std::streamsize>(content_length)) {
        std::cerr << "[JsonRpcProtocol] Failed to read complete JSON payload" << std::endl;
        return "";
    }

    return payload;
}

void JsonRpcProtocol::writeRawMessage(const std::string& json_content) {
    std::lock_guard<std::mutex> lock(write_mutex_);

    if (!connected_) {
        return;
    }

    try {
        // Write Content-Length header
        output_stream_ << "Content-Length: " << json_content.length() << "\r\n\r\n";

        // Write JSON content
        output_stream_ << json_content;

        // Flush to ensure immediate delivery
        output_stream_.flush();

    } catch (const std::exception& e) {
        std::cerr << "[JsonRpcProtocol] Error writing message: " << e.what() << std::endl;
        connected_ = false;
    }
}

bool JsonRpcProtocol::validateJsonRpcMessage(const nlohmann::json& json) const {
    // Must be an object
    if (!json.is_object()) {
        return false;
    }

    // Must have jsonrpc field with value "2.0"
    if (!json.contains("jsonrpc") || json["jsonrpc"] != "2.0") {
        return false;
    }

    // Must have either method (request/notification) or result/error (response)
    bool has_method = json.contains("method");
    bool has_result = json.contains("result");
    bool has_error = json.contains("error");

    if (!has_method && !has_result && !has_error) {
        return false;
    }

    // If it has method, it must be a string
    if (has_method && !json["method"].is_string()) {
        return false;
    }

    // If it has both result and error, it's invalid
    if (has_result && has_error) {
        return false;
    }

    return true;
}

JsonRpcMessageType JsonRpcProtocol::determineMessageType(const nlohmann::json& json) const {
    if (json.contains("method")) {
        // It's a request if it has an id, notification otherwise
        return json.contains("id") ? JsonRpcMessageType::Request : JsonRpcMessageType::Notification;
    } else if (json.contains("error")) {
        return JsonRpcMessageType::Error;
    } else {
        return JsonRpcMessageType::Response;
    }
}

} // namespace core
} // namespace als
