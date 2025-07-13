/**
 * @file test_request_dispatcher.cpp
 * @brief Tests for RequestDispatcher functionality
 */

#include <iostream>
#include <cassert>
#include <chrono>
#include <atomic>
#include <sstream>

#include "als/core/RequestDispatcher.h"
#include "als/core/JsonRpcProtocol.h"
#include "als/core/ThreadPool.h"

using namespace als::core;

/**
 * @brief Test helper for RequestDispatcher testing
 */
class DispatcherTestHelper {
public:
    std::istringstream input;
    std::ostringstream output;
    JsonRpcProtocol protocol;
    ThreadPool thread_pool;
    RequestDispatcher dispatcher;
    
    DispatcherTestHelper() 
        : protocol(input, output)
        , thread_pool(2, 100)
        , dispatcher(protocol, thread_pool) {}
    
    void clearOutput() {
        output.str("");
        output.clear();
    }
    
    std::string getOutput() {
        return output.str();
    }
};

/**
 * @brief Test basic request handler registration and dispatch
 */
void testBasicRequestHandling() {
    std::cout << "[TEST] Testing basic request handling..." << std::endl;
    
    DispatcherTestHelper test;
    
    std::atomic<bool> handler_called{false};
    std::atomic<int> received_id{-1};
    
    // Register a test handler
    test.dispatcher.registerRequestHandler("test/method", [&](const RequestContext& context) {
        handler_called = true;
        received_id = context.request_id.get<int>();
        context.respond({{"result", "success"}});
    });
    
    // Create and dispatch a test request
    nlohmann::json request_json = {
        {"jsonrpc", "2.0"},
        {"id", 123},
        {"method", "test/method"},
        {"params", {{"test", "value"}}}
    };

    JsonRpcRequest request(request_json);
    JsonRpcMessage message(JsonRpcMessageType::Request, request.raw);
    test.dispatcher.dispatch(message);
    
    // Wait for async processing
    test.thread_pool.waitForCompletion(std::chrono::seconds(1));
    
    // Verify handler was called
    assert(handler_called.load());
    assert(received_id.load() == 123);
    
    // Verify response was sent
    std::string output = test.getOutput();
    assert(!output.empty());
    assert(output.find("Content-Length:") != std::string::npos);
    assert(output.find("success") != std::string::npos);
    
    std::cout << "[TEST] Basic request handling passed!" << std::endl;
}

/**
 * @brief Test notification handler registration and dispatch
 */
void testNotificationHandling() {
    std::cout << "[TEST] Testing notification handling..." << std::endl;
    
    DispatcherTestHelper test;
    
    std::atomic<bool> handler_called{false};
    std::string received_method;
    
    // Register a notification handler
    test.dispatcher.registerNotificationHandler("test/notification", [&](const JsonRpcNotification& notification) {
        handler_called = true;
        received_method = notification.method;
    });
    
    // Create and dispatch a test notification
    JsonRpcNotification notification;
    notification.raw = {
        {"jsonrpc", "2.0"},
        {"method", "test/notification"},
        {"params", {{"data", "test"}}}
    };
    notification.method = "test/notification";
    notification.params = {{"data", "test"}};
    
    JsonRpcMessage message(JsonRpcMessageType::Notification, notification.raw);
    test.dispatcher.dispatch(message);
    
    // Wait for async processing
    test.thread_pool.waitForCompletion(std::chrono::seconds(1));
    
    // Verify handler was called
    assert(handler_called.load());
    assert(received_method == "test/notification");
    
    std::cout << "[TEST] Notification handling passed!" << std::endl;
}

/**
 * @brief Test method not found error handling
 */
void testMethodNotFound() {
    std::cout << "[TEST] Testing method not found handling..." << std::endl;
    
    DispatcherTestHelper test;
    
    // Create request for unknown method
    JsonRpcRequest request;
    request.raw = {
        {"jsonrpc", "2.0"},
        {"id", 456},
        {"method", "unknown/method"},
        {"params", {}}
    };
    request.id = 456;
    request.method = "unknown/method";
    request.params = {};
    
    JsonRpcMessage message(JsonRpcMessageType::Request, request.raw);
    test.dispatcher.dispatch(message);
    
    // Wait for async processing
    test.thread_pool.waitForCompletion(std::chrono::seconds(1));
    
    // Verify error response was sent
    std::string output = test.getOutput();
    assert(!output.empty());
    assert(output.find("-32601") != std::string::npos); // Method not found error code
    assert(output.find("Method not found") != std::string::npos);
    
    std::cout << "[TEST] Method not found handling passed!" << std::endl;
}

/**
 * @brief Test request cancellation
 */
void testRequestCancellation() {
    std::cout << "[TEST] Testing request cancellation..." << std::endl;
    
    DispatcherTestHelper test;
    
    std::atomic<bool> handler_started{false};
    std::atomic<bool> handler_completed{false};
    
    // Register a slow handler
    test.dispatcher.registerRequestHandler("slow/method", [&](const RequestContext& context) {
        handler_started = true;
        
        // Simulate slow processing
        for (int i = 0; i < 100; ++i) {
            if (context.cancellation_token && context.cancellation_token->load()) {
                return; // Exit early due to cancellation
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        handler_completed = true;
        context.respond({{"result", "completed"}});
    });
    
    // Create and dispatch request
    JsonRpcRequest request;
    request.raw = {
        {"jsonrpc", "2.0"},
        {"id", 789},
        {"method", "slow/method"},
        {"params", {}}
    };
    request.id = 789;
    request.method = "slow/method";
    request.params = {};
    
    JsonRpcMessage message(JsonRpcMessageType::Request, request.raw);
    test.dispatcher.dispatch(message);
    
    // Wait for handler to start
    while (!handler_started.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Cancel the request
    test.dispatcher.cancelRequest(789);
    
    // Wait a bit more
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Handler should have started but not completed
    assert(handler_started.load());
    assert(!handler_completed.load());
    
    std::cout << "[TEST] Request cancellation passed!" << std::endl;
}

/**
 * @brief Test middleware functionality
 */
void testMiddleware() {
    std::cout << "[TEST] Testing middleware functionality..." << std::endl;
    
    DispatcherTestHelper test;
    
    // Add logging middleware
    test.dispatcher.addMiddleware(std::make_unique<LoggingMiddleware>());
    
    std::atomic<bool> handler_called{false};
    
    // Register a test handler
    test.dispatcher.registerRequestHandler("middleware/test", [&](const RequestContext& context) {
        handler_called = true;
        context.respond({{"result", "middleware_test"}});
    });
    
    // Create and dispatch request
    JsonRpcRequest request;
    request.raw = {
        {"jsonrpc", "2.0"},
        {"id", 999},
        {"method", "middleware/test"},
        {"params", {}}
    };
    request.id = 999;
    request.method = "middleware/test";
    request.params = {};
    
    JsonRpcMessage message(JsonRpcMessageType::Request, request.raw);
    test.dispatcher.dispatch(message);
    
    // Wait for processing
    test.thread_pool.waitForCompletion(std::chrono::seconds(1));
    
    // Verify handler was called
    assert(handler_called.load());
    
    // Verify response was sent
    std::string output = test.getOutput();
    assert(output.find("middleware_test") != std::string::npos);
    
    std::cout << "[TEST] Middleware functionality passed!" << std::endl;
}

/**
 * @brief Test dispatcher statistics
 */
void testStatistics() {
    std::cout << "[TEST] Testing dispatcher statistics..." << std::endl;
    
    DispatcherTestHelper test;
    
    // Register handlers
    test.dispatcher.registerRequestHandler("stats/test", [](const RequestContext& context) {
        context.respond({{"result", "stats"}});
    });
    
    test.dispatcher.registerNotificationHandler("stats/notification", [](const JsonRpcNotification& notification) {
        // Just process the notification
    });
    
    // Send some requests and notifications
    for (int i = 0; i < 3; ++i) {
        JsonRpcRequest request;
        request.raw = {
            {"jsonrpc", "2.0"},
            {"id", i},
            {"method", "stats/test"},
            {"params", {}}
        };
        request.id = i;
        request.method = "stats/test";
        request.params = {};
        
        JsonRpcMessage message(JsonRpcMessageType::Request, request.raw);
        test.dispatcher.dispatch(message);
    }
    
    for (int i = 0; i < 2; ++i) {
        JsonRpcNotification notification;
        notification.raw = {
            {"jsonrpc", "2.0"},
            {"method", "stats/notification"},
            {"params", {}}
        };
        notification.method = "stats/notification";
        notification.params = {};
        
        JsonRpcMessage message(JsonRpcMessageType::Notification, notification.raw);
        test.dispatcher.dispatch(message);
    }
    
    // Wait for processing
    test.thread_pool.waitForCompletion(std::chrono::seconds(1));
    
    // Check statistics
    auto stats = test.dispatcher.getStats();
    assert(stats.total_requests >= 3);
    assert(stats.successful_requests >= 3);
    assert(stats.total_notifications >= 2);
    assert(stats.method_counts["stats/test"] >= 3);
    assert(stats.method_counts["stats/notification"] >= 2);
    
    std::cout << "[TEST] Statistics - Requests: " << stats.total_requests 
              << ", Notifications: " << stats.total_notifications 
              << ", Success: " << stats.successful_requests << std::endl;
    
    std::cout << "[TEST] Dispatcher statistics passed!" << std::endl;
}

/**
 * @brief Main test runner for RequestDispatcher
 */
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "RequestDispatcher Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;
    
    try {
        testBasicRequestHandling();
        testNotificationHandling();
        testMethodNotFound();
        testRequestCancellation();
        testMiddleware();
        testStatistics();
        
        std::cout << "========================================" << std::endl;
        std::cout << "All RequestDispatcher tests passed!" << std::endl;
        std::cout << "========================================" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}
