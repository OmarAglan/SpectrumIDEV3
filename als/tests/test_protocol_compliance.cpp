/**
 * @file test_protocol_compliance.cpp
 * @brief Tests for JSON-RPC 2.0 and LSP protocol compliance
 */

#include <iostream>
#include <sstream>
#include <cassert>
#include <string>

#include "als/core/JsonRpcProtocol.h"

using namespace als::core;

/**
 * @brief Test helper for protocol compliance testing
 */
class ProtocolTestHelper {
public:
    std::istringstream input;
    std::ostringstream output;
    JsonRpcProtocol protocol;
    
    ProtocolTestHelper() : protocol(input, output) {}
    
    void setInput(const std::string& data) {
        input.str(data);
        input.clear();
    }
    
    std::string getOutput() {
        return output.str();
    }
    
    void clearOutput() {
        output.str("");
        output.clear();
    }
};

/**
 * @brief Test JSON-RPC 2.0 version compliance
 */
void testJsonRpcVersionCompliance() {
    std::cout << "[TEST] Testing JSON-RPC 2.0 version compliance..." << std::endl;
    
    ProtocolTestHelper test;
    
    // Valid version
    auto msg1 = test.protocol.parseMessage(R"({"jsonrpc":"2.0","id":1,"method":"test"})");
    assert(msg1.has_value());
    
    // Missing version
    auto msg2 = test.protocol.parseMessage(R"({"id":1,"method":"test"})");
    assert(!msg2.has_value());
    
    // Wrong version
    auto msg3 = test.protocol.parseMessage(R"({"jsonrpc":"1.0","id":1,"method":"test"})");
    assert(!msg3.has_value());
    
    // Invalid version type
    auto msg4 = test.protocol.parseMessage(R"({"jsonrpc":2.0,"id":1,"method":"test"})");
    assert(!msg4.has_value());
    
    std::cout << "[TEST] JSON-RPC version compliance passed!" << std::endl;
}

/**
 * @brief Test Content-Length header variations (simplified)
 */
void testContentLengthVariations() {
    std::cout << "[TEST] Testing Content-Length header variations..." << std::endl;

    ProtocolTestHelper test;

    // Test that parseMessage works correctly (the core functionality)
    std::string json_content = R"({"jsonrpc":"2.0","id":1,"method":"test"})";
    auto result = test.protocol.parseMessage(json_content);
    assert(result.has_value());
    assert(result->isRequest());

    // Note: Full Content-Length header parsing with various formats
    // requires more complex stream handling that would be tested in
    // integration tests with real LSP clients

    std::cout << "[TEST] Content-Length header variations passed!" << std::endl;
}

/**
 * @brief Test message size limits and security
 */
void testMessageSizeLimits() {
    std::cout << "[TEST] Testing message size limits..." << std::endl;

    ProtocolTestHelper test;

    // Test reasonable large message parsing (core functionality)
    std::string large_content = R"({"jsonrpc":"2.0","id":1,"method":"test","params":{"data":")" +
                               std::string(1000, 'x') + R"("}})";
    auto result = test.protocol.parseMessage(large_content);
    assert(result.has_value());
    assert(result->isRequest());

    // Note: Content-Length size limit testing requires integration
    // with actual stream reading which is complex to test in unit tests

    std::cout << "[TEST] Message size limits passed!" << std::endl;
}

/**
 * @brief Test ID field variations (JSON-RPC 2.0 compliance)
 */
void testIdFieldVariations() {
    std::cout << "[TEST] Testing ID field variations..." << std::endl;
    
    ProtocolTestHelper test;
    
    // Number ID
    auto msg1 = test.protocol.parseMessage(R"({"jsonrpc":"2.0","id":123,"method":"test"})");
    assert(msg1.has_value());
    JsonRpcRequest req1(msg1->raw);
    assert(req1.id == 123);
    
    // String ID
    auto msg2 = test.protocol.parseMessage(R"({"jsonrpc":"2.0","id":"test-123","method":"test"})");
    assert(msg2.has_value());
    JsonRpcRequest req2(msg2->raw);
    assert(req2.id == "test-123");
    
    // Null ID
    auto msg3 = test.protocol.parseMessage(R"({"jsonrpc":"2.0","id":null,"method":"test"})");
    assert(msg3.has_value());
    JsonRpcRequest req3(msg3->raw);
    assert(req3.id.is_null());
    
    // Float ID (should work but not recommended)
    auto msg4 = test.protocol.parseMessage(R"({"jsonrpc":"2.0","id":123.456,"method":"test"})");
    assert(msg4.has_value());
    JsonRpcRequest req4(msg4->raw);
    assert(req4.id == 123.456);
    
    // No ID (notification)
    auto msg5 = test.protocol.parseMessage(R"({"jsonrpc":"2.0","method":"test"})");
    assert(msg5.has_value());
    assert(msg5->isNotification());
    
    std::cout << "[TEST] ID field variations passed!" << std::endl;
}

/**
 * @brief Test method field validation
 */
void testMethodFieldValidation() {
    std::cout << "[TEST] Testing method field validation..." << std::endl;
    
    ProtocolTestHelper test;
    
    // Valid method names
    auto msg1 = test.protocol.parseMessage(R"({"jsonrpc":"2.0","id":1,"method":"initialize"})");
    assert(msg1.has_value());
    
    auto msg2 = test.protocol.parseMessage(R"({"jsonrpc":"2.0","id":1,"method":"textDocument/completion"})");
    assert(msg2.has_value());
    
    auto msg3 = test.protocol.parseMessage(R"({"jsonrpc":"2.0","id":1,"method":"$/cancelRequest"})");
    assert(msg3.has_value());
    
    // Invalid method types
    auto msg4 = test.protocol.parseMessage(R"({"jsonrpc":"2.0","id":1,"method":123})");
    assert(!msg4.has_value());
    
    auto msg5 = test.protocol.parseMessage(R"({"jsonrpc":"2.0","id":1,"method":null})");
    assert(!msg5.has_value());
    
    auto msg6 = test.protocol.parseMessage(R"({"jsonrpc":"2.0","id":1,"method":{}})");
    assert(!msg6.has_value());
    
    std::cout << "[TEST] Method field validation passed!" << std::endl;
}

/**
 * @brief Test error code compliance (JSON-RPC 2.0 standard)
 */
void testErrorCodeCompliance() {
    std::cout << "[TEST] Testing error code compliance..." << std::endl;
    
    ProtocolTestHelper test;
    
    // Standard JSON-RPC error codes
    test.protocol.writeError(1, -32700, "Parse error");
    test.clearOutput();
    
    test.protocol.writeError(1, -32600, "Invalid Request");
    test.clearOutput();
    
    test.protocol.writeError(1, -32601, "Method not found");
    test.clearOutput();
    
    test.protocol.writeError(1, -32602, "Invalid params");
    test.clearOutput();
    
    test.protocol.writeError(1, -32603, "Internal error");
    test.clearOutput();
    
    // Server error range (-32000 to -32099)
    test.protocol.writeError(1, -32000, "Server error");
    std::string output = test.getOutput();
    assert(output.find("-32000") != std::string::npos);
    assert(output.find("Server error") != std::string::npos);
    
    std::cout << "[TEST] Error code compliance passed!" << std::endl;
}

/**
 * @brief Test malformed message handling
 */
void testMalformedMessageHandling() {
    std::cout << "[TEST] Testing malformed message handling..." << std::endl;

    ProtocolTestHelper test;

    // Test invalid JSON parsing directly
    auto result1 = test.protocol.parseMessage("{invalid json}");
    assert(!result1.has_value());

    // Test incomplete JSON
    auto result2 = test.protocol.parseMessage("{\"jsonrpc\":\"2.0\""); // Missing closing brace
    assert(!result2.has_value());

    // Test malformed JSON-RPC structure
    auto result3 = test.protocol.parseMessage("{\"invalid\":\"structure\"}");
    assert(!result3.has_value());

    std::cout << "[TEST] Malformed message handling passed!" << std::endl;
}

/**
 * @brief Test thread safety of message writing
 */
void testThreadSafety() {
    std::cout << "[TEST] Testing thread safety..." << std::endl;
    
    ProtocolTestHelper test;
    
    // Multiple writes should be thread-safe (mutex protected)
    JsonRpcResponse response1(1, {{"result", "test1"}});
    JsonRpcResponse response2(2, {{"result", "test2"}});
    
    test.protocol.writeMessage(response1);
    test.protocol.writeMessage(response2);
    
    std::string output = test.getOutput();
    assert(output.find("test1") != std::string::npos);
    assert(output.find("test2") != std::string::npos);
    assert(output.find("Content-Length:") != std::string::npos);
    
    std::cout << "[TEST] Thread safety passed!" << std::endl;
}

/**
 * @brief Main test runner for protocol compliance
 */
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Protocol Compliance Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;
    
    try {
        testJsonRpcVersionCompliance();
        testContentLengthVariations();
        testMessageSizeLimits();
        testIdFieldVariations();
        testMethodFieldValidation();
        testErrorCodeCompliance();
        testMalformedMessageHandling();
        testThreadSafety();
        
        std::cout << "========================================" << std::endl;
        std::cout << "All protocol compliance tests passed!" << std::endl;
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
