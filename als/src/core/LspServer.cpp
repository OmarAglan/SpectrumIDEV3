/**
 * @file LspServer.cpp
 * @brief Implementation of the main LSP server class
 */

#include "als/core/LspServer.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>

namespace als {
namespace core {

/**
 * @brief Private implementation class (PIMPL pattern)
 */
class LspServer::Impl {
public:
    explicit Impl(std::shared_ptr<ServerConfig> config)
        : config_(config), running_(false) {
        // TODO: Initialize components
    }
    
    ~Impl() {
        stop();
    }
    
    bool startStdio() {
        std::cout << "[LspServer] Starting with stdio communication" << std::endl;
        running_ = true;
        return true;
    }

    bool startSocket(int port) {
        std::cout << "[LspServer] Starting with socket on port " << port << std::endl;
        running_ = true;
        return true;
    }
    
    int run() {
        std::cout << "[LspServer] Entering main loop" << std::endl;

        while (running_) {
            try {
                // Read and process one LSP message
                if (!processLspMessage()) {
                    std::cout << "[LspServer] Failed to process message or EOF, exiting" << std::endl;
                    break;
                }
            } catch (const std::exception& e) {
                std::cerr << "[LspServer] Error processing message: " << e.what() << std::endl;
                // Continue processing other messages
            }
        }

        std::cout << "[LspServer] Main loop exited" << std::endl;
        return 0;
    }

    bool processLspMessage() {
        // Step 1: Read headers (looking for Content-Length)
        std::string line;
        int contentLength = -1;

        while (std::getline(std::cin, line)) {
            // Remove \r if present (Windows line endings)
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            // Empty line indicates end of headers
            if (line.empty()) {
                break;
            }

            // Parse Content-Length header
            if (line.find("Content-Length:") == 0) {
                std::string lengthStr = line.substr(15); // Skip "Content-Length:"
                // Trim whitespace
                size_t start = lengthStr.find_first_not_of(" \t");
                if (start != std::string::npos) {
                    lengthStr = lengthStr.substr(start);
                    contentLength = std::stoi(lengthStr);
                    std::cout << "[LspServer] Content-Length: " << contentLength << std::endl;
                }
            }
        }

        // Check if we got EOF while reading headers
        if (std::cin.eof()) {
            return false;
        }

        // Step 2: Read message body if we have content length
        if (contentLength > 0) {
            std::string messageBody(contentLength, '\0');
            std::cin.read(&messageBody[0], contentLength);

            if (std::cin.gcount() != contentLength) {
                std::cerr << "[LspServer] Failed to read complete message body" << std::endl;
                return false;
            }

            std::cout << "[LspServer] Received message: " << messageBody << std::endl;

            // Step 3: Process the JSON-RPC message
            return handleJsonRpcMessage(messageBody);
        } else {
            std::cerr << "[LspServer] No valid Content-Length header found" << std::endl;
            return false;
        }
    }

    bool handleJsonRpcMessage(const std::string& messageBody) {
        try {
            // Parse JSON
            auto message = nlohmann::json::parse(messageBody);

            // Validate JSON-RPC 2.0 format
            if (!message.contains("jsonrpc") || message["jsonrpc"] != "2.0") {
                std::cerr << "[LspServer] Invalid JSON-RPC version" << std::endl;
                return true; // Continue processing other messages
            }

            // Check if it's a request, response, or notification
            if (message.contains("method")) {
                // It's a request or notification
                std::string method = message["method"].get<std::string>();
                std::cout << "[LspServer] Received method: " << method << std::endl;

                // Handle LSP lifecycle methods
                if (method == "initialize") {
                    return handleInitialize(message);
                } else if (method == "initialized") {
                    return handleInitialized(message);
                } else if (method == "shutdown") {
                    return handleShutdown(message);
                } else if (method == "exit") {
                    return handleExit(message);
                } else {
                    // Unknown method - send method not found error if it's a request
                    if (message.contains("id")) {
                        sendErrorResponse(message["id"], -32601, "Method not found", method);
                    }
                    return true;
                }
            } else if (message.contains("result") || message.contains("error")) {
                // It's a response - for now just log it
                std::cout << "[LspServer] Received response: " << message.dump() << std::endl;
                return true;
            } else {
                std::cerr << "[LspServer] Invalid JSON-RPC message format" << std::endl;
                return true;
            }

        } catch (const nlohmann::json::exception& e) {
            std::cerr << "[LspServer] JSON parsing error: " << e.what() << std::endl;
            return true; // Continue processing
        }
    }

    bool handleInitialize(const auto& message) {
        std::cout << "[LspServer] Handling initialize request" << std::endl;

        // Extract client capabilities if present
        if (message.contains("params") && message["params"].contains("capabilities")) {
            std::cout << "[LspServer] Client capabilities received" << std::endl;
        }

        // Send initialize response with server capabilities
        if (message.contains("id")) {
            auto response = nlohmann::json{
                {"jsonrpc", "2.0"},
                {"id", message["id"]},
                {"result", nlohmann::json{
                    {"capabilities", nlohmann::json{
                        {"textDocumentSync", 1}, // Full document sync
                        {"hoverProvider", false},
                        {"completionProvider", nlohmann::json{
                            {"triggerCharacters", nlohmann::json::array()}
                        }},
                        {"definitionProvider", false},
                        {"referencesProvider", false}
                    }},
                    {"serverInfo", nlohmann::json{
                        {"name", "Alif Language Server"},
                        {"version", "1.0.0"}
                    }}
                }}
            };

            sendResponse(response);
        }

        return true;
    }

    bool handleInitialized(const auto& message) {
        (void)message; // Mark as intentionally unused
        std::cout << "[LspServer] Client initialized notification received" << std::endl;
        return true;
    }

    bool handleShutdown(const auto& message) {
        std::cout << "[LspServer] Shutdown request received" << std::endl;

        if (message.contains("id")) {
            auto response = nlohmann::json{
                {"jsonrpc", "2.0"},
                {"id", message["id"]},
                {"result", nullptr}
            };

            sendResponse(response);
        }

        return true;
    }

    bool handleExit(const auto& message) {
        (void)message; // Mark as intentionally unused
        std::cout << "[LspServer] Exit notification received" << std::endl;
        running_ = false;
        return false; // Stop processing
    }

    void sendResponse(const auto& response) {
        std::string responseStr = response.dump();
        std::cout << "Content-Length: " << responseStr.length() << "\r\n\r\n" << responseStr << std::flush;
    }

    void sendErrorResponse(const auto& id, int code, const std::string& message, const std::string& data = "") {
        auto error = nlohmann::json{
            {"jsonrpc", "2.0"},
            {"id", id},
            {"error", nlohmann::json{
                {"code", code},
                {"message", message}
            }}
        };

        if (!data.empty()) {
            error["error"]["data"] = data;
        }

        sendResponse(error);
    }

    void stop() {
        if (running_) {
            std::cout << "[LspServer] Stopping server" << std::endl;
            running_ = false;
        }
    }
    
    bool isRunning() const {
        return running_;
    }

private:
    std::shared_ptr<ServerConfig> config_;
    bool running_;
    
    // TODO: Add actual server components
    // std::unique_ptr<JsonRpcProtocol> protocol_;
    // std::unique_ptr<RequestDispatcher> dispatcher_;
    // std::unique_ptr<ThreadPool> threadPool_;
};

// LspServer implementation

LspServer::LspServer(std::shared_ptr<ServerConfig> config)
    : pImpl(std::make_unique<Impl>(config)) {
}

LspServer::~LspServer() = default;

bool LspServer::startStdio() {
    return pImpl->startStdio();
}

bool LspServer::startSocket(int port) {
    return pImpl->startSocket(port);
}

int LspServer::run() {
    return pImpl->run();
}

void LspServer::stop() {
    pImpl->stop();
}

bool LspServer::isRunning() const {
    return pImpl->isRunning();
}

} // namespace core
} // namespace als
