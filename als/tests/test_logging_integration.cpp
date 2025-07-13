/**
 * @file test_logging_integration.cpp
 * @brief Tests for custom logging library integration
 */

#include <iostream>
#include <cassert>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>

#include "als/logging/Logger.h"

using namespace als::logging;

/**
 * @brief Test basic logging functionality
 */
void testBasicLogging() {
    std::cout << "[TEST] Testing basic logging functionality..." << std::endl;
    
    // Configure logger for console only
    LoggerConfig config;
    config.consoleLevel = LogLevel::Debug;
    config.enableFile = false;
    config.enableConsole = true;
    
    Logger::GetInstance().Configure(config);
    
    // Test all log levels
    ALS_LOG_TRACE("This is a trace message");
    ALS_LOG_DEBUG("This is a debug message");
    ALS_LOG_INFO("This is an info message");
    ALS_LOG_WARN("This is a warning message");
    ALS_LOG_ERROR("This is an error message");
    ALS_LOG_CRITICAL("This is a critical message");
    
    std::cout << "[TEST] Basic logging functionality passed!" << std::endl;
}

/**
 * @brief Test file logging functionality
 */
void testFileLogging() {
    std::cout << "[TEST] Testing file logging functionality..." << std::endl;
    
    const std::string logFile = "test_logging.log";
    
    // Remove existing log file
    if (std::filesystem::exists(logFile)) {
        std::filesystem::remove(logFile);
    }
    
    // Configure logger for file only
    LoggerConfig config;
    config.consoleLevel = LogLevel::Off;
    config.fileLevel = LogLevel::Info;
    config.logFilePath = logFile;
    config.enableFile = true;
    config.enableConsole = false;
    
    Logger::GetInstance().Configure(config);
    
    // Write some log messages
    ALS_LOG_INFO("File logging test message 1");
    ALS_LOG_WARN("File logging test message 2");
    ALS_LOG_ERROR("File logging test message 3");
    
    // Flush to ensure messages are written
    Logger::GetInstance().Flush();

    // Disable file logging to release the file handle
    Logger::GetInstance().EnableFile(false);

    // Check if file was created and contains messages
    assert(std::filesystem::exists(logFile));

    std::ifstream file(logFile);
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();

    assert(content.find("File logging test message 1") != std::string::npos);
    assert(content.find("File logging test message 2") != std::string::npos);
    assert(content.find("File logging test message 3") != std::string::npos);

    // Clean up
    std::filesystem::remove(logFile);
    
    std::cout << "[TEST] File logging functionality passed!" << std::endl;
}

/**
 * @brief Test log level filtering
 */
void testLogLevelFiltering() {
    std::cout << "[TEST] Testing log level filtering..." << std::endl;
    
    const std::string logFile = "test_level_filtering.log";
    
    // Remove existing log file
    if (std::filesystem::exists(logFile)) {
        std::filesystem::remove(logFile);
    }
    
    // Configure logger to only log WARN and above
    LoggerConfig config;
    config.consoleLevel = LogLevel::Off;
    config.fileLevel = LogLevel::Warn;
    config.logFilePath = logFile;
    config.enableFile = true;
    config.enableConsole = false;
    
    Logger::GetInstance().Configure(config);
    
    // Write messages at different levels
    ALS_LOG_TRACE("This should not appear");
    ALS_LOG_DEBUG("This should not appear");
    ALS_LOG_INFO("This should not appear");
    ALS_LOG_WARN("This should appear");
    ALS_LOG_ERROR("This should appear");
    ALS_LOG_CRITICAL("This should appear");
    
    // Flush to ensure messages are written
    Logger::GetInstance().Flush();

    // Disable file logging to release the file handle
    Logger::GetInstance().EnableFile(false);

    // Check file content
    std::ifstream file(logFile);
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();

    // Should contain WARN, ERROR, CRITICAL
    assert(content.find("This should appear") != std::string::npos);
    assert(content.find("[WARN]") != std::string::npos);
    assert(content.find("[ERROR]") != std::string::npos);
    assert(content.find("[CRITICAL]") != std::string::npos);

    // Should NOT contain TRACE, DEBUG, INFO
    assert(content.find("This should not appear") == std::string::npos);
    assert(content.find("[TRACE]") == std::string::npos);
    assert(content.find("[DEBUG]") == std::string::npos);
    assert(content.find("[INFO]") == std::string::npos);

    // Clean up
    std::filesystem::remove(logFile);
    
    std::cout << "[TEST] Log level filtering passed!" << std::endl;
}

/**
 * @brief Test structured logging
 */
void testStructuredLogging() {
    std::cout << "[TEST] Testing structured logging..." << std::endl;
    
    const std::string logFile = "test_structured.log";
    
    // Remove existing log file
    if (std::filesystem::exists(logFile)) {
        std::filesystem::remove(logFile);
    }
    
    // Configure logger for file only
    LoggerConfig config;
    config.consoleLevel = LogLevel::Off;
    config.fileLevel = LogLevel::Info;
    config.logFilePath = logFile;
    config.enableFile = true;
    config.enableConsole = false;
    
    Logger::GetInstance().Configure(config);
    
    // Test structured logging
    std::unordered_map<std::string, std::string> fields = {
        {"request_id", "123"},
        {"method", "initialize"},
        {"duration_ms", "45"}
    };
    
    ALS_LOG_STRUCTURED(LogLevel::Info, "Request processed", fields);
    
    // Flush to ensure messages are written
    Logger::GetInstance().Flush();

    // Disable file logging to release the file handle
    Logger::GetInstance().EnableFile(false);

    // Check file content
    std::ifstream file(logFile);
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();

    assert(content.find("Request processed") != std::string::npos);
    assert(content.find("request_id=123") != std::string::npos);
    assert(content.find("method=initialize") != std::string::npos);
    assert(content.find("duration_ms=45") != std::string::npos);

    // Clean up
    std::filesystem::remove(logFile);
    
    std::cout << "[TEST] Structured logging passed!" << std::endl;
}

/**
 * @brief Test thread safety
 */
void testThreadSafety() {
    std::cout << "[TEST] Testing thread safety..." << std::endl;
    
    const std::string logFile = "test_thread_safety.log";
    
    // Remove existing log file
    if (std::filesystem::exists(logFile)) {
        std::filesystem::remove(logFile);
    }
    
    // Configure logger for file only
    LoggerConfig config;
    config.consoleLevel = LogLevel::Off;
    config.fileLevel = LogLevel::Info;
    config.logFilePath = logFile;
    config.enableFile = true;
    config.enableConsole = false;
    
    Logger::GetInstance().Configure(config);
    
    // Create multiple threads that log simultaneously
    const int numThreads = 4;
    const int messagesPerThread = 10;
    std::vector<std::thread> threads;
    
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([t, messagesPerThread]() {
            for (int i = 0; i < messagesPerThread; ++i) {
                ALS_LOG_INFO("Thread ", t, " message ", i);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Flush to ensure all messages are written
    Logger::GetInstance().Flush();

    // Disable file logging to release the file handle
    Logger::GetInstance().EnableFile(false);

    // Check that we have the expected number of messages
    std::ifstream file(logFile);
    std::string line;
    int messageCount = 0;

    while (std::getline(file, line)) {
        if (line.find("Thread") != std::string::npos && line.find("message") != std::string::npos) {
            messageCount++;
        }
    }
    file.close();

    assert(messageCount == numThreads * messagesPerThread);

    // Clean up
    std::filesystem::remove(logFile);
    
    std::cout << "[TEST] Thread safety passed!" << std::endl;
}

/**
 * @brief Test string to log level conversion
 */
void testStringToLogLevel() {
    std::cout << "[TEST] Testing string to log level conversion..." << std::endl;
    
    assert(StringToLogLevel("trace") == LogLevel::Trace);
    assert(StringToLogLevel("debug") == LogLevel::Debug);
    assert(StringToLogLevel("info") == LogLevel::Info);
    assert(StringToLogLevel("warn") == LogLevel::Warn);
    assert(StringToLogLevel("error") == LogLevel::Error);
    assert(StringToLogLevel("critical") == LogLevel::Critical);
    assert(StringToLogLevel("off") == LogLevel::Off);
    
    // Test case insensitive
    assert(StringToLogLevel("TRACE") == LogLevel::Trace);
    assert(StringToLogLevel("Debug") == LogLevel::Debug);
    assert(StringToLogLevel("INFO") == LogLevel::Info);
    
    // Test invalid input (should default to Info)
    assert(StringToLogLevel("invalid") == LogLevel::Info);
    assert(StringToLogLevel("") == LogLevel::Info);
    
    std::cout << "[TEST] String to log level conversion passed!" << std::endl;
}

/**
 * @brief Main test runner for logging integration
 */
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Custom Logging Integration Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;
    
    try {
        testBasicLogging();
        testFileLogging();
        testLogLevelFiltering();
        testStructuredLogging();
        testThreadSafety();
        testStringToLogLevel();
        
        std::cout << "========================================" << std::endl;
        std::cout << "All logging integration tests passed!" << std::endl;
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
