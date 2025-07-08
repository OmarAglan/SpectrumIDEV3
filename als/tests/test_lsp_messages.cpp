/**
 * @file test_lsp_messages.cpp
 * @brief Tests for LSP-specific message types and real LSP scenarios
 */

#include <iostream>
#include <sstream>
#include <cassert>
#include <string>

#include "als/core/JsonRpcProtocol.h"

using namespace als::core;

/**
 * @brief Test helper for LSP message testing
 */
class LspTestHelper {
public:
    std::istringstream input;
    std::ostringstream output;
    JsonRpcProtocol protocol;
    
    LspTestHelper() : protocol(input, output) {}
    
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
    
    // Helper to create LSP message with Content-Length header
    std::string createLspMessage(const std::string& json) {
        return "Content-Length: " + std::to_string(json.length()) + "\r\n\r\n" + json;
    }
};

/**
 * @brief Test LSP initialize request/response
 */
void testLspInitialize() {
    std::cout << "[TEST] Testing LSP initialize sequence..." << std::endl;
    
    LspTestHelper test;
    
    // Test initialize request
    std::string initialize_request = R"({
        "jsonrpc": "2.0",
        "id": 1,
        "method": "initialize",
        "params": {
            "processId": 12345,
            "clientInfo": {
                "name": "Test Client",
                "version": "1.0.0"
            },
            "capabilities": {
                "textDocument": {
                    "completion": {
                        "completionItem": {
                            "snippetSupport": true
                        }
                    }
                }
            },
            "workspaceFolders": [
                {
                    "uri": "file:///workspace",
                    "name": "Test Workspace"
                }
            ]
        }
    })";
    
    auto message = test.protocol.parseMessage(initialize_request);
    assert(message.has_value());
    assert(message->isRequest());
    
    JsonRpcRequest request(message->raw);
    assert(request.method == "initialize");
    assert(request.id == 1);
    assert(request.params.contains("processId"));
    assert(request.params["processId"] == 12345);
    assert(request.params.contains("clientInfo"));
    assert(request.params["clientInfo"]["name"] == "Test Client");
    
    // Test initialize response
    nlohmann::json capabilities = {
        {"textDocumentSync", 1},
        {"completionProvider", {
            {"triggerCharacters", {".", ":"}}
        }},
        {"hoverProvider", true},
        {"definitionProvider", true}
    };
    
    nlohmann::json result = {
        {"capabilities", capabilities},
        {"serverInfo", {
            {"name", "Alif Language Server"},
            {"version", "1.0.0"}
        }}
    };
    
    JsonRpcResponse response(1, result);
    test.protocol.writeMessage(response);
    
    std::string output = test.getOutput();
    assert(output.find("Content-Length:") != std::string::npos);
    assert(output.find("textDocumentSync") != std::string::npos);
    assert(output.find("Alif Language Server") != std::string::npos);
    
    std::cout << "[TEST] LSP initialize sequence passed!" << std::endl;
}

/**
 * @brief Test LSP text document notifications
 */
void testLspTextDocumentNotifications() {
    std::cout << "[TEST] Testing LSP text document notifications..." << std::endl;
    
    LspTestHelper test;
    
    // Test textDocument/didOpen notification
    std::string did_open = R"({
        "jsonrpc": "2.0",
        "method": "textDocument/didOpen",
        "params": {
            "textDocument": {
                "uri": "file:///test.alif",
                "languageId": "alif",
                "version": 1,
                "text": "دالة اختبار():\n    اطبع(\"مرحبا\")\n"
            }
        }
    })";
    
    auto message1 = test.protocol.parseMessage(did_open);
    assert(message1.has_value());
    assert(message1->isNotification());
    
    JsonRpcNotification notification1(message1->raw);
    assert(notification1.method == "textDocument/didOpen");
    assert(notification1.params.contains("textDocument"));
    assert(notification1.params["textDocument"]["uri"] == "file:///test.alif");
    assert(notification1.params["textDocument"]["languageId"] == "alif");
    
    // Test textDocument/didChange notification
    std::string did_change = R"({
        "jsonrpc": "2.0",
        "method": "textDocument/didChange",
        "params": {
            "textDocument": {
                "uri": "file:///test.alif",
                "version": 2
            },
            "contentChanges": [
                {
                    "range": {
                        "start": {"line": 1, "character": 4},
                        "end": {"line": 1, "character": 10}
                    },
                    "text": "اكتب"
                }
            ]
        }
    })";
    
    auto message2 = test.protocol.parseMessage(did_change);
    assert(message2.has_value());
    assert(message2->isNotification());
    
    JsonRpcNotification notification2(message2->raw);
    assert(notification2.method == "textDocument/didChange");
    assert(notification2.params["textDocument"]["version"] == 2);
    assert(notification2.params.contains("contentChanges"));
    
    std::cout << "[TEST] LSP text document notifications passed!" << std::endl;
}

/**
 * @brief Test LSP completion request/response
 */
void testLspCompletion() {
    std::cout << "[TEST] Testing LSP completion..." << std::endl;
    
    LspTestHelper test;
    
    // Test completion request
    std::string completion_request = R"({
        "jsonrpc": "2.0",
        "id": 2,
        "method": "textDocument/completion",
        "params": {
            "textDocument": {
                "uri": "file:///test.alif"
            },
            "position": {
                "line": 1,
                "character": 4
            },
            "context": {
                "triggerKind": 1
            }
        }
    })";
    
    auto message = test.protocol.parseMessage(completion_request);
    assert(message.has_value());
    assert(message->isRequest());
    
    JsonRpcRequest request(message->raw);
    assert(request.method == "textDocument/completion");
    assert(request.id == 2);
    assert(request.params.contains("position"));
    assert(request.params["position"]["line"] == 1);
    assert(request.params["position"]["character"] == 4);
    
    // Test completion response
    nlohmann::json completion_items = nlohmann::json::array({
        {
            {"label", "اطبع"},
            {"kind", 3}, // Function
            {"detail", "دالة الطباعة"},
            {"documentation", "طباعة النص إلى وحدة التحكم"}
        },
        {
            {"label", "اقرأ"},
            {"kind", 3}, // Function
            {"detail", "دالة القراءة"},
            {"documentation", "قراءة النص من المستخدم"}
        }
    });
    
    JsonRpcResponse response(2, completion_items);
    test.protocol.writeMessage(response);
    
    std::string output = test.getOutput();
    assert(output.find("اطبع") != std::string::npos);
    assert(output.find("اقرأ") != std::string::npos);
    
    std::cout << "[TEST] LSP completion passed!" << std::endl;
}

/**
 * @brief Test LSP error scenarios
 */
void testLspErrors() {
    std::cout << "[TEST] Testing LSP error scenarios..." << std::endl;
    
    LspTestHelper test;
    
    // Test method not found error
    test.protocol.writeError(1, -32601, "Method not found", {{"method", "unknown/method"}});
    std::string output1 = test.getOutput();
    assert(output1.find("-32601") != std::string::npos);
    assert(output1.find("Method not found") != std::string::npos);
    
    test.clearOutput();
    
    // Test invalid params error
    test.protocol.writeError(2, -32602, "Invalid params", {{"expected", "object"}, {"received", "string"}});
    std::string output2 = test.getOutput();
    assert(output2.find("-32602") != std::string::npos);
    assert(output2.find("Invalid params") != std::string::npos);
    
    test.clearOutput();
    
    // Test server error
    test.protocol.writeError(3, -32000, "Server error", {{"details", "Internal processing error"}});
    std::string output3 = test.getOutput();
    assert(output3.find("-32000") != std::string::npos);
    assert(output3.find("Server error") != std::string::npos);
    
    std::cout << "[TEST] LSP error scenarios passed!" << std::endl;
}

/**
 * @brief Test LSP workspace notifications
 */
void testLspWorkspaceNotifications() {
    std::cout << "[TEST] Testing LSP workspace notifications..." << std::endl;
    
    LspTestHelper test;
    
    // Test workspace/didChangeConfiguration
    std::string config_change = R"({
        "jsonrpc": "2.0",
        "method": "workspace/didChangeConfiguration",
        "params": {
            "settings": {
                "alif": {
                    "maxNumberOfProblems": 100,
                    "trace": {
                        "server": "verbose"
                    }
                }
            }
        }
    })";
    
    auto message = test.protocol.parseMessage(config_change);
    assert(message.has_value());
    assert(message->isNotification());
    
    JsonRpcNotification notification(message->raw);
    assert(notification.method == "workspace/didChangeConfiguration");
    assert(notification.params.contains("settings"));
    assert(notification.params["settings"]["alif"]["maxNumberOfProblems"] == 100);
    
    std::cout << "[TEST] LSP workspace notifications passed!" << std::endl;
}

/**
 * @brief Test full LSP message parsing (without Content-Length complexity)
 */
void testFullLspMessage() {
    std::cout << "[TEST] Testing full LSP message parsing..." << std::endl;

    LspTestHelper test;

    // Test direct JSON parsing for complex LSP message
    std::string json_content = R"({"jsonrpc":"2.0","id":1,"method":"textDocument/hover","params":{"textDocument":{"uri":"file:///test.alif"},"position":{"line":0,"character":5}}})";
    auto message = test.protocol.parseMessage(json_content);

    assert(message.has_value());
    assert(message->isRequest());

    JsonRpcRequest request(message->raw);
    assert(request.method == "textDocument/hover");
    assert(request.params["position"]["line"] == 0);
    assert(request.params["position"]["character"] == 5);

    std::cout << "[TEST] Full LSP message passed!" << std::endl;
}

/**
 * @brief Main test runner for LSP messages
 */
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "LSP Message Test Suite" << std::endl;
    std::cout << "========================================" << std::endl;
    
    try {
        testLspInitialize();
        testLspTextDocumentNotifications();
        testLspCompletion();
        testLspErrors();
        testLspWorkspaceNotifications();
        testFullLspMessage();
        
        std::cout << "========================================" << std::endl;
        std::cout << "All LSP message tests passed!" << std::endl;
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
