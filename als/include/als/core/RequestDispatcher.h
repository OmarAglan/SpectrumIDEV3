/**
 * @file RequestDispatcher.h
 * @brief Request dispatcher for LSP method routing and handling
 */

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <atomic>
#include <chrono>

#include "als/core/JsonRpcProtocol.h"
#include "als/core/ThreadPool.h"

namespace als {
namespace core {

// Forward declarations
class JsonRpcProtocol;
class ThreadPool;

/**
 * @brief Callback for sending responses
 */
using ResponseCallback = std::function<void(const nlohmann::json& result)>;

/**
 * @brief Callback for sending errors
 */
using ErrorCallback = std::function<void(int code, const std::string& message, const nlohmann::json& data)>;

/**
 * @brief Context for request handling
 */
struct RequestContext {
    JsonRpcId request_id;
    std::string method;
    nlohmann::json params;
    ResponseCallback respond;
    ErrorCallback error;
    std::shared_ptr<std::atomic<bool>> cancellation_token;
    std::chrono::steady_clock::time_point start_time;
};

/**
 * @brief Handler function for LSP requests
 */
using RequestHandler = std::function<void(const RequestContext& context)>;

/**
 * @brief Handler function for LSP notifications
 */
using NotificationHandler = std::function<void(const JsonRpcNotification& notification)>;

/**
 * @brief Middleware interface for request processing
 */
class RequestMiddleware {
public:
    virtual ~RequestMiddleware() = default;
    
    /**
     * @brief Process request before handler execution
     * @param context Request context
     * @return true to continue processing, false to stop
     */
    virtual bool preProcess(const RequestContext& context) = 0;
    
    /**
     * @brief Process request after handler execution
     * @param context Request context
     * @param success Whether handler executed successfully
     */
    virtual void postProcess(const RequestContext& context, bool success) = 0;
};

/**
 * @brief Statistics for request processing
 */
struct DispatcherStats {
    size_t total_requests = 0;
    size_t successful_requests = 0;
    size_t failed_requests = 0;
    size_t cancelled_requests = 0;
    size_t total_notifications = 0;
    std::chrono::milliseconds total_processing_time{0};
    std::chrono::milliseconds average_processing_time{0};
    std::unordered_map<std::string, size_t> method_counts;
};

/**
 * @brief Request dispatcher for LSP method routing and handling
 * 
 * Provides:
 * - Method-based routing for LSP requests and notifications
 * - Request validation and parameter extraction
 * - Error handling and response formatting
 * - Middleware support for logging and metrics
 * - Cancellation support for long-running operations
 * - Integration with ThreadPool for async processing
 */
class RequestDispatcher {
public:
    /**
     * @brief Constructor
     * @param protocol JSON-RPC protocol for sending responses
     * @param thread_pool Thread pool for async processing
     */
    RequestDispatcher(JsonRpcProtocol& protocol, ThreadPool& thread_pool);
    
    /**
     * @brief Destructor
     */
    ~RequestDispatcher();
    
    // Non-copyable, non-movable
    RequestDispatcher(const RequestDispatcher&) = delete;
    RequestDispatcher& operator=(const RequestDispatcher&) = delete;
    RequestDispatcher(RequestDispatcher&&) = delete;
    RequestDispatcher& operator=(RequestDispatcher&&) = delete;
    
    /**
     * @brief Register a handler for LSP requests
     * @param method LSP method name (e.g., "textDocument/completion")
     * @param handler Handler function
     */
    void registerRequestHandler(const std::string& method, RequestHandler handler);
    
    /**
     * @brief Register a handler for LSP notifications
     * @param method LSP method name (e.g., "textDocument/didOpen")
     * @param handler Handler function
     */
    void registerNotificationHandler(const std::string& method, NotificationHandler handler);
    
    /**
     * @brief Add middleware for request processing
     * @param middleware Middleware instance
     */
    void addMiddleware(std::unique_ptr<RequestMiddleware> middleware);
    
    /**
     * @brief Dispatch an LSP message to appropriate handler
     * @param message JSON-RPC message to dispatch
     */
    void dispatch(const JsonRpcMessage& message);
    
    /**
     * @brief Cancel a specific request
     * @param request_id Request ID to cancel
     */
    void cancelRequest(const JsonRpcId& request_id);
    
    /**
     * @brief Cancel all pending requests
     */
    void cancelAllRequests();
    
    /**
     * @brief Get dispatcher statistics
     * @return Current statistics
     */
    DispatcherStats getStats() const;
    
    /**
     * @brief Reset statistics
     */
    void resetStats();
    
    /**
     * @brief Check if a method has a registered handler
     * @param method Method name to check
     * @return true if handler exists
     */
    bool hasRequestHandler(const std::string& method) const;
    bool hasNotificationHandler(const std::string& method) const;

private:
    // Core components
    JsonRpcProtocol& protocol_;
    ThreadPool& thread_pool_;
    
    // Handler registries
    std::unordered_map<std::string, RequestHandler> request_handlers_;
    std::unordered_map<std::string, NotificationHandler> notification_handlers_;
    
    // Middleware
    std::vector<std::unique_ptr<RequestMiddleware>> middleware_;
    
    // Request tracking
    std::unordered_map<JsonRpcId, std::shared_ptr<std::atomic<bool>>> active_requests_;
    mutable std::mutex requests_mutex_;
    
    // Statistics
    mutable std::mutex stats_mutex_;
    DispatcherStats stats_;
    
    // Internal methods
    void dispatchRequest(const JsonRpcRequest& request);
    void dispatchNotification(const JsonRpcNotification& notification);
    
    void executeRequestHandler(const RequestContext& context, const RequestHandler& handler);
    void executeNotificationHandler(const JsonRpcNotification& notification, const NotificationHandler& handler);
    
    bool runMiddlewarePreProcess(const RequestContext& context);
    void runMiddlewarePostProcess(const RequestContext& context, bool success);
    
    void sendMethodNotFoundError(const JsonRpcId& id, const std::string& method);
    void sendInternalError(const JsonRpcId& id, const std::string& message);
    
    void updateStats(const std::string& method, bool success, std::chrono::milliseconds duration);
};

/**
 * @brief Logging middleware for request processing
 */
class LoggingMiddleware : public RequestMiddleware {
public:
    bool preProcess(const RequestContext& context) override;
    void postProcess(const RequestContext& context, bool success) override;
};

/**
 * @brief Metrics middleware for request processing
 */
class MetricsMiddleware : public RequestMiddleware {
public:
    bool preProcess(const RequestContext& context) override;
    void postProcess(const RequestContext& context, bool success) override;
    
    struct Metrics {
        size_t total_requests = 0;
        std::chrono::milliseconds total_time{0};
        std::unordered_map<std::string, size_t> method_counts;
    };
    
    Metrics getMetrics() const;
    void resetMetrics();

private:
    mutable std::mutex metrics_mutex_;
    Metrics metrics_;
};

} // namespace core
} // namespace als
