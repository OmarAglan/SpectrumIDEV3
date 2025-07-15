/**
 * @file LspServer.cpp
 * @brief Implementation of the main LSP server class
 */

#include "als/core/LspServer.h"
#include "als/core/JsonRpcProtocol.h"
#include "als/core/ThreadPool.h"
#include "als/core/RequestDispatcher.h"
#include "../features/CompletionProvider.h"
#include "als/logging/Logger.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <memory>

// Socket includes
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#endif

namespace als {
namespace core {

/**
 * @brief Simple socket stream wrapper for JsonRpcProtocol
 */
class SocketStream : public std::iostream {
private:
    class SocketStreamBuf : public std::streambuf {
    private:
        int socket_fd_;
        char* buffer_;
        static const size_t BUFFER_SIZE = 4096;

    public:
        explicit SocketStreamBuf(int socket_fd) : socket_fd_(socket_fd) {
            buffer_ = new char[BUFFER_SIZE];
            setg(buffer_, buffer_, buffer_); // Set get area
            setp(buffer_, buffer_ + BUFFER_SIZE); // Set put area
        }

        ~SocketStreamBuf() {
            delete[] buffer_;
        }

    protected:
        // Input operations
        int underflow() override {
            if (gptr() < egptr()) {
                return traits_type::to_int_type(*gptr());
            }

#ifdef _WIN32
            int bytes_read = recv(socket_fd_, buffer_, BUFFER_SIZE, 0);
#else
            ssize_t bytes_read = read(socket_fd_, buffer_, BUFFER_SIZE);
#endif

            if (bytes_read <= 0) {
                return traits_type::eof();
            }

            setg(buffer_, buffer_, buffer_ + bytes_read);
            return traits_type::to_int_type(*gptr());
        }

        // Output operations
        int overflow(int ch) override {
            if (sync() == -1) {
                return traits_type::eof();
            }

            if (ch != traits_type::eof()) {
                *pptr() = static_cast<char>(ch);
                pbump(1);
            }

            return ch;
        }

        int sync() override {
            size_t bytes_to_write = pptr() - pbase();
            if (bytes_to_write > 0) {
#ifdef _WIN32
                int bytes_written = send(socket_fd_, pbase(), static_cast<int>(bytes_to_write), 0);
#else
                ssize_t bytes_written = write(socket_fd_, pbase(), bytes_to_write);
#endif

                if (bytes_written != static_cast<int>(bytes_to_write)) {
                    return -1;
                }

                setp(buffer_, buffer_ + BUFFER_SIZE);
            }
            return 0;
        }
    };

    std::unique_ptr<SocketStreamBuf> socket_buf_;

public:
    explicit SocketStream(int socket_fd)
        : std::iostream(nullptr), socket_buf_(std::make_unique<SocketStreamBuf>(socket_fd)) {
        rdbuf(socket_buf_.get());
    }
};

/**
 * @brief Private implementation class (PIMPL pattern)
 */
class LspServer::Impl {
public:
    explicit Impl(std::shared_ptr<ServerConfig> config)
        : config_(config), running_(false), protocol_(std::cin, std::cout),
          use_socket_(false), socket_port_(-1), server_socket_(-1), client_socket_(-1) {
        // Initialize ThreadPool
        threadPool_ = std::make_unique<ThreadPool>(4, 1000); // 4 threads, max 1000 queued tasks
        ALS_LOG_INFO("ThreadPool initialized with 4 threads and max 1000 queued tasks");

        // Initialize RequestDispatcher
        dispatcher_ = std::make_unique<RequestDispatcher>(protocol_, *threadPool_);

        // Initialize CompletionProvider
        completionProvider_ = std::make_unique<features::CompletionProvider>();

        // Add middleware
        dispatcher_->addMiddleware(std::make_unique<LoggingMiddleware>());
        dispatcher_->addMiddleware(std::make_unique<MetricsMiddleware>());

        // Register LSP handlers
        registerLspHandlers();

        ALS_LOG_INFO("RequestDispatcher initialized with middleware and completion provider");
    }
    
    ~Impl() {
        stop();
        cleanupSockets();
    }
    
    bool startStdio() {
        ALS_LOG_INFO("Starting LSP server with stdio communication");
        running_ = true;
        return true;
    }

    bool startSocket(int port) {
        ALS_LOG_INFO("Starting LSP server with socket on port ", port);

        use_socket_ = true;
        socket_port_ = port;

        // Initialize Winsock on Windows
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            ALS_LOG_CRITICAL("WSAStartup failed");
            return false;
        }
#endif

        // Create socket
#ifdef _WIN32
        SOCKET temp_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (temp_socket == INVALID_SOCKET) {
            ALS_LOG_CRITICAL("Failed to create socket");
            return false;
        }
        server_socket_ = static_cast<int>(temp_socket);
#else
        server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket_ < 0) {
            ALS_LOG_CRITICAL("Failed to create socket");
            return false;
        }
#endif

        // Set socket options
        int opt = 1;
#ifdef _WIN32
        if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR,
                      reinterpret_cast<const char*>(&opt), sizeof(opt)) < 0) {
#else
        if (setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
#endif
            ALS_LOG_ERROR("Failed to set socket options");
        }

        // Bind socket
        sockaddr_in address;
        address.sin_family = AF_INET;
#ifdef _WIN32
        inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);
#else
        address.sin_addr.s_addr = inet_addr("127.0.0.1"); // Localhost only
#endif
        address.sin_port = htons(static_cast<uint16_t>(port));

        if (bind(server_socket_, reinterpret_cast<struct sockaddr*>(&address), sizeof(address)) < 0) {
            ALS_LOG_CRITICAL("Failed to bind socket to port ", port);
            cleanupSockets();
            return false;
        }

        // Listen for connections
        if (listen(server_socket_, 1) < 0) {
            ALS_LOG_CRITICAL("Failed to listen on socket");
            cleanupSockets();
            return false;
        }

        ALS_LOG_INFO("Socket server listening on port ", port);
        running_ = true;
        return true;
    }
    
    int run() {
        if (use_socket_) {
            return runSocket();
        } else {
            return runStdio();
        }
    }

private:
    int runStdio() {
        ALS_LOG_INFO("Entering LSP server main loop (stdio)");

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

    int runSocket() {
        ALS_LOG_INFO("Entering LSP server main loop (socket)");

        // Accept client connection
        ALS_LOG_INFO("Waiting for client connection...");
#ifdef _WIN32
        SOCKET temp_client = accept(static_cast<SOCKET>(server_socket_), nullptr, nullptr);
        if (temp_client == INVALID_SOCKET) {
            ALS_LOG_CRITICAL("Failed to accept client connection");
            return 1;
        }
        client_socket_ = static_cast<int>(temp_client);
#else
        client_socket_ = accept(server_socket_, nullptr, nullptr);
        if (client_socket_ < 0) {
            ALS_LOG_CRITICAL("Failed to accept client connection");
            return 1;
        }
#endif

        ALS_LOG_INFO("Client connected successfully");

        // Create socket stream and new protocol instance
        auto socket_stream = std::make_unique<SocketStream>(client_socket_);
        JsonRpcProtocol socket_protocol(*socket_stream, *socket_stream);

        // Update dispatcher to use socket protocol
        dispatcher_ = std::make_unique<RequestDispatcher>(socket_protocol, *threadPool_);

        // Re-register handlers with the new dispatcher
        registerLspHandlers();

        // Process messages using socket protocol
        while (running_ && socket_protocol.isConnected()) {
            try {
                auto message = socket_protocol.readMessage();
                if (!message.has_value()) {
                    ALS_LOG_INFO("No message received or client disconnected, exiting main loop");
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

        ALS_LOG_INFO("LSP server socket main loop exited");
        return 0;
    }

public:

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

            // Clean up socket resources
            cleanupSockets();
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
    std::unique_ptr<features::CompletionProvider> completionProvider_;

    // Socket-related members
    bool use_socket_;
    int socket_port_;
    int server_socket_;
    int client_socket_;

    // LSP handler registration
    void registerLspHandlers();

    // LSP request handlers
    void handleInitializeRequest(const RequestContext& context);
    void handleShutdownRequest(const RequestContext& context);
    void handleCompletionRequest(const RequestContext& context);

    // LSP notification handlers
    void handleDidOpenNotification(const JsonRpcNotification& notification);
    void handleDidChangeNotification(const JsonRpcNotification& notification);
    void handleDidCloseNotification(const JsonRpcNotification& notification);
    void handleExitNotification(const JsonRpcNotification& notification);

    // Socket cleanup
    void cleanupSockets() {
        if (client_socket_ >= 0) {
#ifdef _WIN32
            closesocket(client_socket_);
#else
            close(client_socket_);
#endif
            client_socket_ = -1;
        }

        if (server_socket_ >= 0) {
#ifdef _WIN32
            closesocket(server_socket_);
#else
            close(server_socket_);
#endif
            server_socket_ = -1;
        }

#ifdef _WIN32
        if (use_socket_) {
            WSACleanup();
        }
#endif
    }
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

    // Register completion request handler
    dispatcher_->registerRequestHandler("textDocument/completion", [this](const RequestContext& context) {
        handleCompletionRequest(context);
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
                {"triggerCharacters", nlohmann::json::array({".", " ", "(", "[", "{"})}
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

void LspServer::Impl::handleCompletionRequest(const RequestContext& context) {
    ALS_LOG_INFO("Handling textDocument/completion request");

    try {
        // Extract parameters from request
        if (!context.params.contains("textDocument") || !context.params.contains("position")) {
            context.error(-32602, "Invalid params: missing textDocument or position", nlohmann::json{});
            return;
        }

        auto textDocument = context.params["textDocument"];
        auto position = context.params["position"];

        if (!textDocument.contains("uri") || !position.contains("line") || !position.contains("character")) {
            context.error(-32602, "Invalid params: missing uri, line, or character", nlohmann::json{});
            return;
        }

        std::string uri = textDocument["uri"].get<std::string>();
        size_t line = position["line"].get<size_t>();
        size_t character = position["character"].get<size_t>();

        ALS_LOG_DEBUG("Completion request for ", uri, " at ", line, ":", character);

        // For now, provide basic completion without document content
        // In a full implementation, we would retrieve document content from a document manager
        std::string documentContent = ""; // TODO: Get actual document content

        // Create enhanced completion context for Arabic completions
        auto completionContext = completionProvider_->createArabicContext(uri, documentContent, line, character);

        // Get enhanced Arabic completions
        auto arabicCompletions = completionProvider_->provideArabicCompletions(completionContext);

        // Convert Arabic completions to JSON format
        nlohmann::json result = nlohmann::json::array();
        for (const auto& item : arabicCompletions) {
            result.push_back(item.toJson());
        }

        // Wrap in completion list format
        nlohmann::json completionList = nlohmann::json{
            {"isIncomplete", false},
            {"items", result}
        };

        context.respond(completionList);

        ALS_LOG_DEBUG("Provided ", arabicCompletions.size(), " Arabic completion items");

    } catch (const std::exception& e) {
        ALS_LOG_ERROR("Error in completion request: ", e.what());
        context.error(-32603, "Internal error", nlohmann::json{{"details", e.what()}});
    }
}

} // namespace core
} // namespace als
