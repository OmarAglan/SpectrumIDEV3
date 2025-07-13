/**
 * @file RequestDispatcher.cpp
 * @brief Request dispatcher implementation for LSP method routing and handling
 */

#include "als/core/RequestDispatcher.h"
#include "als/logging/Logger.h"
#include <iostream>
#include <algorithm>

namespace als {
namespace core {

RequestDispatcher::RequestDispatcher(JsonRpcProtocol& protocol, ThreadPool& thread_pool)
    : protocol_(protocol), thread_pool_(thread_pool) {
    ALS_LOG_INFO("RequestDispatcher initialized");
}

RequestDispatcher::~RequestDispatcher() {
    ALS_LOG_INFO("RequestDispatcher shutting down");
    cancelAllRequests();
}

void RequestDispatcher::registerRequestHandler(const std::string& method, RequestHandler handler) {
    ALS_LOG_DEBUG("Registering request handler for method: ", method);
    request_handlers_[method] = std::move(handler);
}

void RequestDispatcher::registerNotificationHandler(const std::string& method, NotificationHandler handler) {
    ALS_LOG_DEBUG("Registering notification handler for method: ", method);
    notification_handlers_[method] = std::move(handler);
}

void RequestDispatcher::addMiddleware(std::unique_ptr<RequestMiddleware> middleware) {
    ALS_LOG_DEBUG("Adding middleware to RequestDispatcher");
    middleware_.push_back(std::move(middleware));
}

void RequestDispatcher::dispatch(const JsonRpcMessage& message) {
    if (message.isRequest()) {
        JsonRpcRequest request(message.raw);
        dispatchRequest(request);
    } else if (message.isNotification()) {
        JsonRpcNotification notification(message.raw);
        dispatchNotification(notification);
    } else {
        ALS_LOG_DEBUG("Ignoring response/error message");
    }
}

void RequestDispatcher::dispatchRequest(const JsonRpcRequest& request) {
    ALS_LOG_DEBUG("Dispatching request: ", request.method);

    // Check if handler exists
    auto handler_it = request_handlers_.find(request.method);
    if (handler_it == request_handlers_.end()) {
        sendMethodNotFoundError(request.id, request.method);
        return;
    }

    // Create cancellation token
    auto cancellation_token = thread_pool_.createCancellationToken();

    // Track active request
    {
        std::lock_guard<std::mutex> lock(requests_mutex_);
        active_requests_[request.id] = cancellation_token;
    }

    // Create request context
    RequestContext context{
        request.id,
        request.method,
        request.params,
        [this, id = request.id](const nlohmann::json& result) {
            JsonRpcResponse response(id, result);
            protocol_.writeMessage(response);
        },
        [this, id = request.id](int code, const std::string& message, const nlohmann::json& data) {
            protocol_.writeError(id, code, message, data);
        },
        cancellation_token,
        std::chrono::steady_clock::now()
    };

    // Submit to thread pool for async execution
    thread_pool_.submit(ThreadPool::TaskPriority::NORMAL, [this, context, handler = handler_it->second]() {
        executeRequestHandler(context, handler);
    });
}

void RequestDispatcher::dispatchNotification(const JsonRpcNotification& notification) {
    ALS_LOG_DEBUG("Dispatching notification: ", notification.method);

    // Check if handler exists
    auto handler_it = notification_handlers_.find(notification.method);
    if (handler_it == notification_handlers_.end()) {
        ALS_LOG_WARN("No handler registered for notification: ", notification.method);
        return;
    }

    // Submit to thread pool for async execution (lower priority for notifications)
    thread_pool_.submit(ThreadPool::TaskPriority::LOW, [this, notification, handler = handler_it->second]() {
        executeNotificationHandler(notification, handler);
    });
}

void RequestDispatcher::executeRequestHandler(const RequestContext& context, const RequestHandler& handler) {
    auto start_time = std::chrono::steady_clock::now();
    bool success = false;

    try {
        // Check for cancellation before processing
        if (context.cancellation_token && context.cancellation_token->load()) {
            ALS_LOG_DEBUG("Request cancelled before execution: ", context.method);
            {
                std::lock_guard<std::mutex> lock(stats_mutex_);
                stats_.cancelled_requests++;
            }
            return;
        }

        // Run pre-processing middleware
        if (!runMiddlewarePreProcess(context)) {
            ALS_LOG_WARN("Request blocked by middleware: ", context.method);
            return;
        }

        // Execute the handler
        handler(context);
        success = true;

    } catch (const std::exception& e) {
        std::cerr << "[RequestDispatcher] Request handler failed: " << e.what() << std::endl;
        context.error(-32603, "Internal error", nlohmann::json{{"details", e.what()}});
    } catch (...) {
        std::cerr << "[RequestDispatcher] Request handler failed with unknown exception" << std::endl;
        context.error(-32603, "Internal error", nlohmann::json{{"details", "Unknown exception"}});
    }

    // Calculate processing time
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // Run post-processing middleware
    runMiddlewarePostProcess(context, success);

    // Update statistics
    updateStats(context.method, success, duration);

    // Remove from active requests
    {
        std::lock_guard<std::mutex> lock(requests_mutex_);
        active_requests_.erase(context.request_id);
    }
}

void RequestDispatcher::executeNotificationHandler(const JsonRpcNotification& notification, const NotificationHandler& handler) {
    auto start_time = std::chrono::steady_clock::now();
    bool success = false;

    try {
        handler(notification);
        success = true;

    } catch (const std::exception& e) {
        std::cerr << "[RequestDispatcher] Notification handler failed: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "[RequestDispatcher] Notification handler failed with unknown exception" << std::endl;
    }

    // Calculate processing time
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    // Update statistics
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.total_notifications++;
        updateStats(notification.method, success, duration);
    }
}

void RequestDispatcher::cancelRequest(const JsonRpcId& request_id) {
    std::lock_guard<std::mutex> lock(requests_mutex_);
    auto it = active_requests_.find(request_id);
    if (it != active_requests_.end()) {
        it->second->store(true);
        ALS_LOG_DEBUG("Cancelled request: ", request_id.dump());
    }
}

void RequestDispatcher::cancelAllRequests() {
    std::lock_guard<std::mutex> lock(requests_mutex_);
    for (auto& [id, token] : active_requests_) {
        token->store(true);
    }
    ALS_LOG_INFO("Cancelled ", active_requests_.size(), " active requests");
    active_requests_.clear();
}

bool RequestDispatcher::runMiddlewarePreProcess(const RequestContext& context) {
    for (auto& middleware : middleware_) {
        if (!middleware->preProcess(context)) {
            return false;
        }
    }
    return true;
}

void RequestDispatcher::runMiddlewarePostProcess(const RequestContext& context, bool success) {
    for (auto& middleware : middleware_) {
        middleware->postProcess(context, success);
    }
}

void RequestDispatcher::sendMethodNotFoundError(const JsonRpcId& id, const std::string& method) {
    protocol_.writeError(id, -32601, "Method not found", nlohmann::json{{"method", method}});

    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.failed_requests++;
}

void RequestDispatcher::sendInternalError(const JsonRpcId& id, const std::string& message) {
    protocol_.writeError(id, -32603, "Internal error", nlohmann::json{{"details", message}});

    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.failed_requests++;
}

void RequestDispatcher::updateStats(const std::string& method, bool success, std::chrono::milliseconds duration) {
    // Note: This method assumes stats_mutex_ is already locked by the caller
    if (success) {
        stats_.successful_requests++;
    } else {
        stats_.failed_requests++;
    }

    stats_.total_requests++;
    stats_.total_processing_time += duration;
    if (stats_.total_requests > 0) {
        stats_.average_processing_time = stats_.total_processing_time / stats_.total_requests;
    }

    stats_.method_counts[method]++;
}

DispatcherStats RequestDispatcher::getStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void RequestDispatcher::resetStats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_ = DispatcherStats{};
}

bool RequestDispatcher::hasRequestHandler(const std::string& method) const {
    return request_handlers_.find(method) != request_handlers_.end();
}

bool RequestDispatcher::hasNotificationHandler(const std::string& method) const {
    return notification_handlers_.find(method) != notification_handlers_.end();
}

// LoggingMiddleware implementation
bool LoggingMiddleware::preProcess(const RequestContext& context) {
    ALS_LOG_DEBUG("Processing request: ", context.method, " (ID: ", context.request_id.dump(), ")");
    return true;
}

void LoggingMiddleware::postProcess(const RequestContext& context, bool success) {
    auto duration = std::chrono::steady_clock::now() - context.start_time;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

    ALS_LOG_DEBUG("Completed request: ", context.method,
                  " (ID: ", context.request_id.dump(), ") ",
                  "Success: ", (success ? "true" : "false"),
                  " Duration: ", ms.count(), "ms");
}

// MetricsMiddleware implementation
bool MetricsMiddleware::preProcess(const RequestContext& context) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    metrics_.total_requests++;
    metrics_.method_counts[context.method]++;
    return true;
}

void MetricsMiddleware::postProcess(const RequestContext& context, bool success) {
    (void)success; // Mark as intentionally unused
    auto duration = std::chrono::steady_clock::now() - context.start_time;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

    std::lock_guard<std::mutex> lock(metrics_mutex_);
    metrics_.total_time += ms;
}

MetricsMiddleware::Metrics MetricsMiddleware::getMetrics() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    return metrics_;
}

void MetricsMiddleware::resetMetrics() {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    metrics_ = Metrics{};
}

} // namespace core
} // namespace als
