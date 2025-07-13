/**
 * @file LspServer.cpp
 * @brief Implementation of the main LSP server class
 */

#include "als/core/LspServer.h"
#include "als/core/JsonRpcProtocol.h"
#include "als/core/ThreadPool.h"
#include "als/core/RequestDispatcher.h"
#include "als/logging/Logger.h"
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
        ALS_LOG_INFO("ThreadPool initialized with 4 threads and max 1000 queued tasks");

        // Initialize RequestDispatcher
        dispatcher_ = std::make_unique<RequestDispatcher>(protocol_, *threadPool_);

        // Add middleware
        dispatcher_->addMiddleware(std::make_unique<LoggingMiddleware>());
        dispatcher_->addMiddleware(std::make_unique<MetricsMiddleware>());

        // Register LSP handlers
        registerLspHandlers();

        ALS_LOG_INFO("RequestDispatcher initialized with middleware");
    }
    
    ~Impl() {
        stop();
    }
    
    bool startStdio() {
        ALS_LOG_INFO("Starting LSP server with stdio communication");
        running_ = true;
        return true;
    }

    bool startSocket(int port) {
        ALS_LOG_INFO("Starting LSP server with socket on port ", port);
        running_ = true;
        return true;
    }
    
    int run() {
        ALS_LOG_INFO("Entering LSP server main loop");

        while (running_ && protocol_.isConnected()) {
            try {
                // Read and process one LSP message using JsonRpcProtocol
                auto message = protocol_.readMessage();
                if (!message.has_value()) {
                    ALS_LOG_INFO("No message received or EOF, exiting main loop");
                    break;
                }

                if (!handleMessage(message.value())) {
                    ALS_LOG_INFO("Message handling requested exit");
                    break;
                }

            } catch (const std::exception& e) {
                ALS_LOG_ERROR("Error processing message: ", e.what());
                // Continue processing other messages
            }
        }

        ALS_LOG_INFO("LSP server main loop exited");
        return 0;
    }

    bool handleMessage(const JsonRpcMessage& message) {
        ALS_LOG_DEBUG("Processing message type: ", static_cast<int>(message.type));

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
            ALS_LOG_INFO("Stopping LSP server");
            running_ = false;

            // Wait for ThreadPool to complete current tasks
            if (threadPool_) {
                ALS_LOG_INFO("Waiting for ThreadPool to complete tasks...");
                threadPool_->waitForCompletion(std::chrono::seconds(5));
                auto stats = threadPool_->getStats();
                ALS_LOG_INFO("ThreadPool final stats - Submitted: ", stats.submitted,
                           ", Completed: ", stats.completed,
                           ", Cancelled: ", stats.cancelled,
                           ", Failed: ", stats.failed);
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
    std::unique_ptr<RequestDispatcher> dispatcher_;

    // LSP handler registration
    void registerLspHandlers();

    // LSP request handlers
    void handleInitializeRequest(const RequestContext& context);
    void handleShutdownRequest(const RequestContext& context);

    // LSP notification handlers
    void handleDidOpenNotification(const JsonRpcNotification& notification);
    void handleDidChangeNotification(const JsonRpcNotification& notification);
    void handleDidCloseNotification(const JsonRpcNotification& notification);
    void handleExitNotification(const JsonRpcNotification& notification);
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

// Implementation of Impl methods
void LspServer::Impl::registerLspHandlers() {
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

    ALS_LOG_INFO("LSP handlers registered successfully");
}

void LspServer::Impl::handleInitializeRequest(const RequestContext& context) {
    ALS_LOG_INFO("Handling LSP initialize request");

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

void LspServer::Impl::handleShutdownRequest(const RequestContext& context) {
    ALS_LOG_INFO("Handling LSP shutdown request");
    running_ = false;
    context.respond(nlohmann::json{});
}

void LspServer::Impl::handleDidOpenNotification(const JsonRpcNotification& notification) {
    (void)notification; // Mark as intentionally unused
    ALS_LOG_DEBUG("Handling textDocument/didOpen notification");
    // TODO: Parse and store document
}

void LspServer::Impl::handleDidChangeNotification(const JsonRpcNotification& notification) {
    (void)notification; // Mark as intentionally unused
    ALS_LOG_DEBUG("Handling textDocument/didChange notification");
    // TODO: Update document content
}

void LspServer::Impl::handleDidCloseNotification(const JsonRpcNotification& notification) {
    (void)notification; // Mark as intentionally unused
    ALS_LOG_DEBUG("Handling textDocument/didClose notification");
    // TODO: Remove document from memory
}

void LspServer::Impl::handleExitNotification(const JsonRpcNotification& notification) {
    (void)notification; // Mark as intentionally unused
    ALS_LOG_INFO("Handling LSP exit notification");
    running_ = false;
}

} // namespace core
} // namespace als
