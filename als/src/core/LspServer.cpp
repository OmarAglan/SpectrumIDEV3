/**
 * @file LspServer.cpp
 * @brief Implementation of the main LSP server class
 */

#include "als/core/LspServer.h"
#include "als/core/JsonRpcProtocol.h"
#include "als/core/ThreadPool.h"
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
        : config_(config), running_(false), protocol_(std::cin, std::cout) {
        // Initialize ThreadPool
        threadPool_ = std::make_unique<ThreadPool>(4, 1000); // 4 threads, max 1000 queued tasks
        std::cout << "[LspServer] ThreadPool initialized" << std::endl;
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

        while (running_ && protocol_.isConnected()) {
            try {
                // Read and process one LSP message using JsonRpcProtocol
                auto message = protocol_.readMessage();
                if (!message.has_value()) {
                    std::cout << "[LspServer] No message received or EOF, exiting" << std::endl;
                    break;
                }

                if (!handleMessage(message.value())) {
                    std::cout << "[LspServer] Message handling requested exit" << std::endl;
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

    bool handleMessage(const JsonRpcMessage& message) {
        std::cout << "[LspServer] Processing message type: " << static_cast<int>(message.type) << std::endl;

        if (message.isRequest()) {
            JsonRpcRequest request(message.raw);
            return handleRequest(request);
        } else if (message.isNotification()) {
            JsonRpcNotification notification(message.raw);
            return handleNotification(notification);
        } else {
            // Response or error - for now just log
            std::cout << "[LspServer] Received response/error: " << message.raw.dump() << std::endl;
            return true;
        }
    }

    bool handleRequest(const JsonRpcRequest& request) {
        std::cout << "[LspServer] Handling request: " << request.method << std::endl;

        if (request.method == "initialize") {
            return handleInitialize(request);
        } else if (request.method == "shutdown") {
            return handleShutdown(request);
        } else {
            // Unknown method - send method not found error
            protocol_.writeError(request.id, -32601, "Method not found", request.method);
            return true;
        }
    }

    bool handleNotification(const JsonRpcNotification& notification) {
        std::cout << "[LspServer] Handling notification: " << notification.method << std::endl;

        if (notification.method == "initialized") {
            return handleInitialized(notification);
        } else if (notification.method == "exit") {
            return handleExit(notification);
        } else {
            // Unknown notification - just ignore
            std::cout << "[LspServer] Ignoring unknown notification: " << notification.method << std::endl;
            return true;
        }
    }



    bool handleInitialize(const JsonRpcRequest& request) {
        std::cout << "[LspServer] Handling initialize request" << std::endl;

        // Extract client capabilities if present
        if (request.params.contains("capabilities")) {
            std::cout << "[LspServer] Client capabilities received" << std::endl;
        }

        // Create initialize response with server capabilities
        auto result = nlohmann::json{
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
        };

        JsonRpcResponse response(request.id, result);
        protocol_.writeMessage(response);

        return true;
    }

    bool handleInitialized(const JsonRpcNotification& notification) {
        (void)notification; // Mark as intentionally unused
        std::cout << "[LspServer] Client initialized notification received" << std::endl;
        return true;
    }

    bool handleShutdown(const JsonRpcRequest& request) {
        std::cout << "[LspServer] Shutdown request received" << std::endl;

        JsonRpcResponse response(request.id, nullptr);
        protocol_.writeMessage(response);

        return true;
    }

    bool handleExit(const JsonRpcNotification& notification) {
        (void)notification; // Mark as intentionally unused
        std::cout << "[LspServer] Exit notification received" << std::endl;
        running_ = false;
        return false; // Stop processing
    }



    void stop() {
        if (running_) {
            std::cout << "[LspServer] Stopping server" << std::endl;
            running_ = false;

            // Wait for ThreadPool to complete current tasks
            if (threadPool_) {
                std::cout << "[LspServer] Waiting for ThreadPool to complete tasks..." << std::endl;
                threadPool_->waitForCompletion(std::chrono::seconds(5));
                auto stats = threadPool_->getStats();
                std::cout << "[LspServer] ThreadPool stats - Submitted: " << stats.submitted
                         << ", Completed: " << stats.completed
                         << ", Cancelled: " << stats.cancelled
                         << ", Failed: " << stats.failed << std::endl;
            }
        }
    }
    
    bool isRunning() const {
        return running_;
    }

private:
    std::shared_ptr<ServerConfig> config_;
    bool running_;
    JsonRpcProtocol protocol_;
    std::unique_ptr<ThreadPool> threadPool_;

    // TODO: Add other server components
    // std::unique_ptr<RequestDispatcher> dispatcher_;
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
