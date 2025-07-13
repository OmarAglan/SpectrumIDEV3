/**
 * @file LspServer.cpp
 * @brief Implementation of the main LSP server class
 */

#include "als/core/LspServer.h"
#include "als/core/JsonRpcProtocol.h"
#include "als/core/ThreadPool.h"
#include "als/core/RequestDispatcher.h"
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

        // Initialize RequestDispatcher
        dispatcher_ = std::make_unique<RequestDispatcher>(protocol_, *threadPool_);

        // Add middleware
        dispatcher_->addMiddleware(std::make_unique<LoggingMiddleware>());
        dispatcher_->addMiddleware(std::make_unique<MetricsMiddleware>());

        // Register LSP handlers
        registerLspHandlers();

        std::cout << "[LspServer] RequestDispatcher initialized" << std::endl;
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

        // Use RequestDispatcher for all message handling
        dispatcher_->dispatch(message);

        // Check for shutdown/exit requests to determine if we should continue
        if (message.isRequest()) {
            JsonRpcRequest request(message.raw);
            if (request.method == "shutdown" || request.method == "exit") {
                return false; // Signal to exit main loop
            }
        } else if (message.isNotification()) {
            JsonRpcNotification notification(message.raw);
            if (notification.method == "exit") {
                return false; // Signal to exit main loop
            }
        }

        return true;
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

    void registerLspHandlers() {
        // Register initialize request handler
        dispatcher_->registerRequestHandler("initialize", [this](const RequestContext& context) {
            handleInitializeRequest(context);
        });

        // Register shutdown request handler
        dispatcher_->registerRequestHandler("shutdown", [this](const RequestContext& context) {
            handleShutdownRequest(context);
        });

        // Register textDocument/didOpen notification handler
        dispatcher_->registerNotificationHandler("textDocument/didOpen", [this](const JsonRpcNotification& notification) {
            handleDidOpenNotification(notification);
        });

        // Register textDocument/didChange notification handler
        dispatcher_->registerNotificationHandler("textDocument/didChange", [this](const JsonRpcNotification& notification) {
            handleDidChangeNotification(notification);
        });

        // Register textDocument/didClose notification handler
        dispatcher_->registerNotificationHandler("textDocument/didClose", [this](const JsonRpcNotification& notification) {
            handleDidCloseNotification(notification);
        });

        // Register exit notification handler
        dispatcher_->registerNotificationHandler("exit", [this](const JsonRpcNotification& notification) {
            handleExitNotification(notification);
        });

        std::cout << "[LspServer] LSP handlers registered" << std::endl;
    }

    void handleInitializeRequest(const RequestContext& context) {
        std::cout << "[LspServer] Handling initialize request" << std::endl;

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

        context.respond(result);
    }

    void handleShutdownRequest(const RequestContext& context) {
        std::cout << "[LspServer] Handling shutdown request" << std::endl;
        running_ = false;
        context.respond(nlohmann::json{});
    }

    void handleDidOpenNotification(const JsonRpcNotification& notification) {
        std::cout << "[LspServer] Handling textDocument/didOpen notification" << std::endl;
        // TODO: Parse and store document
    }

    void handleDidChangeNotification(const JsonRpcNotification& notification) {
        std::cout << "[LspServer] Handling textDocument/didChange notification" << std::endl;
        // TODO: Update document content
    }

    void handleDidCloseNotification(const JsonRpcNotification& notification) {
        std::cout << "[LspServer] Handling textDocument/didClose notification" << std::endl;
        // TODO: Remove document from memory
    }

    void handleExitNotification(const JsonRpcNotification& notification) {
        std::cout << "[LspServer] Handling exit notification" << std::endl;
        running_ = false;
    }

private:
    std::shared_ptr<ServerConfig> config_;
    bool running_;
    JsonRpcProtocol protocol_;
    std::unique_ptr<ThreadPool> threadPool_;
    std::unique_ptr<RequestDispatcher> dispatcher_;

    // LSP handler registration
    void registerLspHandlers();
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
