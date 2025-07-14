/**
 * @file JsonRpcProtocol.cpp
 * @brief JSON-RPC 2.0 protocol implementation for LSP communication
 */

#include "als/core/JsonRpcProtocol.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <iomanip>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#endif

namespace als {
namespace core {

JsonRpcProtocol::JsonRpcProtocol(std::istream& input, std::ostream& output)
    : input_stream_(input)
    , output_stream_(output)
    , connected_(true) {
    configureInputStreamForWindows();
}

void JsonRpcProtocol::configureInputStreamForWindows() {
#ifdef _WIN32
    // Set stdin to binary mode to prevent CRLF translation
    if (&input_stream_ == &std::cin) {
        _setmode(_fileno(stdin), _O_BINARY);

        // Disable input buffering
        std::cin.tie(nullptr);
        std::ios::sync_with_stdio(false);

        // Set console input to UTF-8 if available
        SetConsoleCP(CP_UTF8);
        SetConsoleOutputCP(CP_UTF8);
    }
#endif
}

std::string JsonRpcProtocol::readLine() {
    std::string line;
    char ch;

    while (input_stream_.get(ch)) {
        if (ch == '\r') {
            // Check if next char is \n
            if (input_stream_.peek() == '\n') {
                input_stream_.get(); // consume the \n
            }
            break;
        } else if (ch == '\n') {
            break;
        } else {
            line += ch;
        }
    }

    return line;
}

size_t JsonRpcProtocol::parseContentLength(const std::string& header) {
    const std::string prefix = "Content-Length: ";
    if (header.find(prefix) == 0) {
        std::string length_str = header.substr(prefix.length());
        // Remove any trailing whitespace
        length_str.erase(length_str.find_last_not_of(" \t\r\n") + 1);

        try {
            return std::stoull(length_str);
        } catch (const std::exception&) {
            std::cerr << "[JsonRpcProtocol] Invalid Content-Length: "
                     << length_str << std::endl;
            return 0;
        }
    }
    return 0;
}

bool JsonRpcProtocol::isJsonComplete(const std::string& json) {
    int brace_count = 0;
    bool in_string = false;
    bool escaped = false;

    for (char ch : json) {
        if (escaped) {
            escaped = false;
            continue;
        }

        if (ch == '\\' && in_string) {
            escaped = true;
            continue;
        }

        if (ch == '"') {
            in_string = !in_string;
            continue;
        }

        if (!in_string) {
            if (ch == '{') brace_count++;
            else if (ch == '}') brace_count--;
        }
    }

    return brace_count == 0 && !in_string;
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
    size_t content_length = 0;

    // Read headers until empty line using enhanced readLine()
    while (true) {
        line = readLine();

        if (line.empty()) {
            break; // Empty line indicates end of headers
        }

        // Parse Content-Length header using enhanced parser
        size_t length = parseContentLength(line);
        if (length > 0) {
            content_length = length;
        }
    }

    if (content_length == 0) {
        std::cerr << "[JsonRpcProtocol] No valid Content-Length found" << std::endl;
        return std::nullopt;
    }

    // Return the content length as a string (this is what the calling code expects)
    return std::to_string(content_length);
}

std::string JsonRpcProtocol::readJsonPayload(size_t content_length) {
    // Enhanced payload reading with character-by-character approach
    std::string payload;
    payload.reserve(content_length);

    char ch;
    size_t bytes_read = 0;

    while (bytes_read < content_length && input_stream_.get(ch)) {
        payload += ch;
        bytes_read++;
    }

    if (bytes_read != content_length) {
        std::cerr << "[JsonRpcProtocol] Failed to read complete JSON payload. "
                 << "Expected: " << content_length << " bytes, "
                 << "Read: " << bytes_read << " bytes" << std::endl;

        // Debug: show what we actually read
        std::cerr << "[JsonRpcProtocol] Partial payload: " << payload << std::endl;

        // For debugging: show hex dump of what we read
        std::cerr << "[JsonRpcProtocol] Hex dump: ";
        for (size_t i = 0; i < payload.length(); ++i) {
            std::cerr << std::hex << std::setw(2) << std::setfill('0')
                     << (unsigned char)payload[i] << " ";
        }
        std::cerr << std::dec << std::endl;

        return "";
    }

    // Validate JSON completeness
    if (!isJsonComplete(payload)) {
        std::cerr << "[JsonRpcProtocol] JSON appears incomplete: " << payload << std::endl;
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
