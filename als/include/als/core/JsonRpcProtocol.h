/**
 * @file JsonRpcProtocol.h
 * @brief JSON-RPC 2.0 protocol implementation for LSP communication
 */

#pragma once

#include <iostream>
#include <optional>
#include <string>
#include <mutex>
#include <functional>
#include <nlohmann/json.hpp>

namespace als {
namespace core {

/**
 * @brief JSON-RPC message types
 */
enum class JsonRpcMessageType {
    Request,
    Response,
    Notification,
    Error
};

/**
 * @brief JSON-RPC request ID type (can be string, number, or null)
 */
using JsonRpcId = nlohmann::json;

/**
 * @brief Base class for JSON-RPC messages
 */
struct JsonRpcMessage {
    JsonRpcMessageType type;
    std::string jsonrpc = "2.0";
    nlohmann::json raw; // Original JSON for access to all fields
    
    JsonRpcMessage(JsonRpcMessageType t, const nlohmann::json& json) 
        : type(t), raw(json) {}
    
    bool isRequest() const { return type == JsonRpcMessageType::Request; }
    bool isResponse() const { return type == JsonRpcMessageType::Response; }
    bool isNotification() const { return type == JsonRpcMessageType::Notification; }
    bool isError() const { return type == JsonRpcMessageType::Error; }
};

/**
 * @brief JSON-RPC request message
 */
struct JsonRpcRequest : public JsonRpcMessage {
    JsonRpcId id;
    std::string method;
    nlohmann::json params;
    
    JsonRpcRequest(const nlohmann::json& json) 
        : JsonRpcMessage(JsonRpcMessageType::Request, json)
        , id(json["id"])
        , method(json["method"].get<std::string>())
        , params(json.contains("params") ? json["params"] : nlohmann::json{}) {}
};

/**
 * @brief JSON-RPC notification message
 */
struct JsonRpcNotification : public JsonRpcMessage {
    std::string method;
    nlohmann::json params;
    
    JsonRpcNotification(const nlohmann::json& json)
        : JsonRpcMessage(JsonRpcMessageType::Notification, json)
        , method(json["method"].get<std::string>())
        , params(json.contains("params") ? json["params"] : nlohmann::json{}) {}
};

/**
 * @brief JSON-RPC response message
 */
struct JsonRpcResponse : public JsonRpcMessage {
    JsonRpcId id;
    nlohmann::json result;
    
    JsonRpcResponse(const JsonRpcId& request_id, const nlohmann::json& response_result)
        : JsonRpcMessage(JsonRpcMessageType::Response, nlohmann::json{})
        , id(request_id)
        , result(response_result) {
        raw = {
            {"jsonrpc", "2.0"},
            {"id", id},
            {"result", result}
        };
    }
};

/**
 * @brief JSON-RPC error message
 */
struct JsonRpcError : public JsonRpcMessage {
    JsonRpcId id;
    int code;
    std::string message;
    nlohmann::json data;
    
    JsonRpcError(const JsonRpcId& request_id, int error_code, const std::string& error_message, 
                 const nlohmann::json& error_data = nlohmann::json{})
        : JsonRpcMessage(JsonRpcMessageType::Error, nlohmann::json{})
        , id(request_id)
        , code(error_code)
        , message(error_message)
        , data(error_data) {
        raw = {
            {"jsonrpc", "2.0"},
            {"id", id},
            {"error", {
                {"code", code},
                {"message", message}
            }}
        };
        if (!data.is_null()) {
            raw["error"]["data"] = data;
        }
    }
};

/**
 * @brief JSON-RPC protocol handler for LSP communication
 * 
 * Handles JSON-RPC 2.0 protocol communication over stdio streams.
 * Provides message parsing, validation, and serialization capabilities.
 */
class JsonRpcProtocol {
public:
    /**
     * @brief Constructor
     * @param input Input stream (typically std::cin)
     * @param output Output stream (typically std::cout)
     */
    JsonRpcProtocol(std::istream& input, std::ostream& output);
    
    /**
     * @brief Destructor
     */
    ~JsonRpcProtocol() = default;
    
    // Non-copyable
    JsonRpcProtocol(const JsonRpcProtocol&) = delete;
    JsonRpcProtocol& operator=(const JsonRpcProtocol&) = delete;
    
    // Message reading
    std::optional<JsonRpcMessage> readMessage();
    std::optional<JsonRpcMessage> parseMessage(const std::string& json_content);
    
    // Message writing
    void writeMessage(const JsonRpcResponse& response);
    void writeMessage(const JsonRpcNotification& notification);
    void writeMessage(const JsonRpcError& error);
    
    // Convenience methods
    void writeError(const JsonRpcId& id, int code, const std::string& message, 
                   const nlohmann::json& data = nlohmann::json{});
    void writeParseError(const std::string& message);
    
    // Protocol state
    bool isConnected() const;
    void disconnect();

private:
    std::istream& input_stream_;
    std::ostream& output_stream_;
    mutable std::mutex write_mutex_;
    bool connected_;
    
    // Internal message handling
    std::optional<std::string> readContentLengthHeader();
    std::string readJsonPayload(size_t content_length);
    void writeRawMessage(const std::string& json_content);

    // Windows-specific input stream configuration
    void configureInputStreamForWindows();

    // Enhanced reading methods for Windows compatibility
    std::string readLine();
    size_t parseContentLength(const std::string& header);
    bool isJsonComplete(const std::string& json);

    // Message validation
    bool validateJsonRpcMessage(const nlohmann::json& json) const;
    JsonRpcMessageType determineMessageType(const nlohmann::json& json) const;
};

} // namespace core
} // namespace als
