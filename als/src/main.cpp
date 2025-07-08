/**
 * @file main.cpp
 * @brief Entry point for the Alif Language Server (ALS)
 * 
 * This file contains the main function that initializes and runs the
 * Alif Language Server, implementing the Language Server Protocol (LSP)
 * for the Alif programming language.
 */

#include <iostream>
#include <string>
#include <memory>

#include "als/core/LspServer.h"
#include "als/core/ServerConfig.h"
// TODO: Include Logger when implemented
// #include "als/core/Logger.h"

/**
 * @brief Print usage information
 */
void printUsage(const char* programName) {
    std::cout << "Alif Language Server (ALS) v1.0.0\n";
    std::cout << "Usage: " << programName << " [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --stdio              Use stdio for communication (default)\n";
    std::cout << "  --socket PORT        Use socket on specified port\n";
    std::cout << "  --log-file FILE      Log to specified file\n";
    std::cout << "  --log-level LEVEL    Set log level (trace|debug|info|warn|error)\n";
    std::cout << "  --config FILE        Use specified configuration file\n";
    std::cout << "  --version            Show version information\n";
    std::cout << "  --help               Show this help message\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << programName << "                    # Start with stdio\n";
    std::cout << "  " << programName << " --socket 8080      # Start with socket\n";
    std::cout << "  " << programName << " --log-file als.log # Log to file\n";
}

/**
 * @brief Print version information
 */
void printVersion() {
    std::cout << "Alif Language Server (ALS) v1.0.0\n";
    std::cout << "Built with C++23\n";
    std::cout << "Language Server Protocol v3.17\n";
    std::cout << "Copyright (c) 2025 ALS Project\n";
}

/**
 * @brief Parse command line arguments
 */
struct CommandLineArgs {
    bool useStdio = true;
    int socketPort = -1;
    std::string logFile;
    std::string logLevel = "info";
    std::string configFile;
    bool showHelp = false;
    bool showVersion = false;
};

CommandLineArgs parseArgs(int argc, char* argv[]) {
    CommandLineArgs args;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            args.showHelp = true;
        } else if (arg == "--version" || arg == "-v") {
            args.showVersion = true;
        } else if (arg == "--stdio") {
            args.useStdio = true;
        } else if (arg == "--socket" && i + 1 < argc) {
            args.useStdio = false;
            args.socketPort = std::stoi(argv[++i]);
        } else if (arg == "--log-file" && i + 1 < argc) {
            args.logFile = argv[++i];
        } else if (arg == "--log-level" && i + 1 < argc) {
            args.logLevel = argv[++i];
        } else if (arg == "--config" && i + 1 < argc) {
            args.configFile = argv[++i];
        } else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            args.showHelp = true;
        }
    }
    
    return args;
}

/**
 * @brief Main entry point
 */
int main(int argc, char* argv[]) {
    try {
        // Parse command line arguments
        auto args = parseArgs(argc, argv);
        
        if (args.showHelp) {
            printUsage(argv[0]);
            return 0;
        }
        
        if (args.showVersion) {
            printVersion();
            return 0;
        }
        
        // TODO: Initialize logging system
        std::cout << "Initializing Alif Language Server..." << std::endl;

        // TODO: Initialize logging system
        std::cout << "Log level: " << args.logLevel << std::endl;

        // Load configuration
        if (!args.configFile.empty()) {
            std::cout << "Loading configuration from: " << args.configFile << std::endl;
        }

        std::cout << "Setting up server components..." << std::endl;
        
        // Initialize server components
        auto config = std::make_shared<als::core::ServerConfig>();
        if (!args.configFile.empty()) {
            config->loadFromFile(args.configFile);
        }

        auto server = std::make_unique<als::core::LspServer>(config);

        // Start server
        if (args.useStdio) {
            std::cout << "Starting LSP server with stdio communication..." << std::endl;
            server->startStdio();
        } else {
            std::cout << "Starting LSP server on port " << args.socketPort << "..." << std::endl;
            server->startSocket(args.socketPort);
        }

        // Run server main loop
        std::cout << "Entering main server loop..." << std::endl;
        int exitCode = server->run();

        std::cout << "Server shutting down with exit code: " << exitCode << std::endl;

        return exitCode;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown error occurred" << std::endl;
        return 1;
    }
}
