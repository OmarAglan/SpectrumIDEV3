/**
 * @file LspServer.h
 * @brief Main LSP server class for the Alif Language Server
 */

#pragma once

#include <memory>
#include <string>

namespace als {
namespace core {

class ServerConfig;
class JsonRpcProtocol;
class RequestDispatcher;
class ThreadPool;

/**
 * @brief Main Language Server Protocol server implementation
 * 
 * This class manages the overall LSP server lifecycle, including:
 * - Communication protocol handling
 * - Request dispatching
 * - Server state management
 * - Graceful shutdown
 */
class LspServer {
public:
    /**
     * @brief Construct LSP server with configuration
     * @param config Server configuration
     */
    explicit LspServer(std::shared_ptr<ServerConfig> config);
    
    /**
     * @brief Destructor
     */
    ~LspServer();
    
    /**
     * @brief Start server with stdio communication
     * @return true if started successfully
     */
    bool startStdio();
    
    /**
     * @brief Start server with socket communication
     * @param port Port number to listen on
     * @return true if started successfully
     */
    bool startSocket(int port);
    
    /**
     * @brief Run the server main loop
     * @return Exit code (0 for success)
     */
    int run();
    
    /**
     * @brief Stop the server gracefully
     */
    void stop();
    
    /**
     * @brief Check if server is running
     * @return true if running
     */
    bool isRunning() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace core
} // namespace als
