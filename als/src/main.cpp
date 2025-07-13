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
#include "als/logging/Logger.h"

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
        
        // Initialize logging system
        als::logging::LoggerConfig logConfig;
        logConfig.consoleLevel = als::logging::StringToLogLevel(args.logLevel);
        logConfig.fileLevel = als::logging::LogLevel::Debug;

        if (!args.logFile.empty()) {
            logConfig.logFilePath = args.logFile;
            logConfig.enableFile = true;
        } else {
            logConfig.enableFile = false;
        }

        als::logging::Logger::GetInstance().Configure(logConfig);

        ALS_LOG_INFO("Initializing Alif Language Server v1.0.0");
        ALS_LOG_INFO("Log level set to: ", args.logLevel);

        // Load configuration
        if (!args.configFile.empty()) {
            ALS_LOG_INFO("Loading configuration from: ", args.configFile);
        }

        ALS_LOG_INFO("Setting up server components...");
        
        // Initialize server components
        auto config = std::make_shared<als::core::ServerConfig>();
        if (!args.configFile.empty()) {
            config->loadFromFile(args.configFile);
        }

        auto server = std::make_unique<als::core::LspServer>(config);

        // Start server
        if (args.useStdio) {
            ALS_LOG_INFO("Starting LSP server with stdio communication");
            server->startStdio();
        } else {
            ALS_LOG_INFO("Starting LSP server on port ", args.socketPort);
            server->startSocket(args.socketPort);
        }

        // Run server main loop
        ALS_LOG_INFO("Entering main server loop");
        int exitCode = server->run();

        ALS_LOG_INFO("Server shutting down with exit code: ", exitCode);

        return exitCode;

    } catch (const std::exception& e) {
        ALS_LOG_CRITICAL("Fatal error: ", e.what());
        return 1;
    } catch (...) {
        ALS_LOG_CRITICAL("Unknown fatal error occurred");
        return 1;
    }
}
