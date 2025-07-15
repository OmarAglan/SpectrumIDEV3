
### **Roadmap: Migrating from Stdio to TCP Sockets**

The goal is to change the communication channel from the fragile standard input/output pipes to a reliable local network connection.

---

### **Phase 1: Modify the Server (`als` project) to Listen on a Socket**

First, we must enable the Alif Language Server to start in "socket mode." It will act as a TCP server, listening for a single connection from the IDE.

#### **Task 1.1: Update Server's `main()` to Parse Socket Arguments**

The server needs to understand a new command-line argument, `--socket <port>`, to know it should start in socket mode.

**File to Edit**: `als/src/main.cpp`

**Action**: Modify the `CommandLineArgs` struct and the `parseArgs` function to handle the new option.

```cpp
// In als/src/main.cpp

// ... (previous code)

struct CommandLineArgs {
    bool useStdio = true; // Default remains stdio for now
    int socketPort = -1;
    // ... other args
};

CommandLineArgs parseArgs(int argc, char* argv[]) {
    CommandLineArgs args;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--socket" && i + 1 < argc) {
            args.useStdio = false;
            args.socketPort = std::stoi(argv[++i]);
        } 
        // ... other argument parsing
    }
    return args;
}

int main(int argc, char* argv[]) {
    // ... (argument parsing)
    auto args = parseArgs(argc, argv);

    // ... (logging and config setup)

    auto server = std::make_unique<als::core::LspServer>(config);

    // Start server based on mode
    if (args.useStdio) {
        ALS_LOG_INFO("Starting LSP server with stdio communication");
        server->startStdio();
    } else {
        if (args.socketPort <= 0) {
            ALS_LOG_CRITICAL("Invalid port provided for socket mode.");
            return 1;
        }
        ALS_LOG_INFO("Starting LSP server on socket port ", args.socketPort);
        if (!server->startSocket(args.socketPort)) { // <-- We will implement this next
            ALS_LOG_CRITICAL("Failed to start server on socket.");
            return 1;
        }
    }

    int exitCode = server->run();
    // ...
}
```

#### **Task 1.2: Implement Socket Listening in `LspServer`**

This is the most critical server-side change. We need to add the logic to create a listening socket, accept a connection, and then use that connection for communication.

**File to Edit**: `als/src/core/LspServer.cpp`

**Context**: We will add platform-specific code for creating sockets. The `run()` method will now have two paths: the old stdio path and the new socket path.

**Action**: Add the necessary headers and implement the `startSocket` and socket-handling logic in `run()`.

```cpp
// Add new headers at the top of als/src/core/LspServer.cpp
#include <vector>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

// Inside LspServer::Impl class

class LspServer::Impl {
public:
    // ... existing members
    int socket_port_ = -1;
    bool use_socket_ = false;

    // ... existing constructor

    bool startSocket(int port) {
        use_socket_ = true;
        socket_port_ = port;
        return true;
    }
    
    int run() {
        if (use_socket_) {
            return runSocket();
        }
        return runStdio();
    }

private:
    int runStdio() {
        ALS_LOG_INFO("Entering LSP server main loop (stdio)");
        // ... (existing run() logic for stdio)
        return 0;
    }

    int runSocket() {
        ALS_LOG_INFO("Entering LSP server main loop (socket)");
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            ALS_LOG_CRITICAL("WSAStartup failed.");
            return 1;
        }
#endif
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            ALS_LOG_CRITICAL("Socket creation failed.");
            return 1;
        }

        sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(socket_port_);

        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            ALS_LOG_CRITICAL("Socket bind failed on port ", socket_port_);
            return 1;
        }

        if (listen(server_fd, 1) < 0) {
            ALS_LOG_CRITICAL("Socket listen failed.");
            return 1;
        }

        ALS_LOG_INFO("Server listening on port ", socket_port_);
        int client_socket = accept(server_fd, nullptr, nullptr);
        if (client_socket < 0) {
            ALS_LOG_CRITICAL("Socket accept failed.");
            return 1;
        }

        ALS_LOG_INFO("Client connected.");
        
        // This is a simplified stream wrapper. A more robust solution would be better.
        // For now, we assume JsonRpcProtocol can be adapted to use file descriptors.
        // Or better yet, we adapt JsonRpcProtocol to take a socket.
        // For now, we will assume this conceptual step. The actual implementation
        // would require a custom streambuf.

        // The key is that communication now happens over client_socket, not std::cin.
        // Since implementing a full C++ streambuf from a socket is complex,
        // we'll assume for this roadmap that we can adapt JsonRpcProtocol.
        
        // The loop would then read from the client_socket instead of std::cin.
        // For brevity, we'll omit the full custom streambuf code.
        
        // ... The message loop would now use the client_socket ...

#ifdef _WIN32
        closesocket(server_fd);
        WSACleanup();
#else
        close(server_fd);
#endif
        return 0;
    }
    // ... rest of the implementation
};
```
*Note: A complete implementation requires a custom `streambuf`. The above is a conceptual guide showing where the socket logic goes.*

---

### **Phase 2: Modify the Client (`Spectrum` project) to Use Sockets**

The client will now be responsible for finding a free port, launching the server with that port, and connecting to it.

#### **Task 2.1: Add `QTcpSocket` to `SpectrumLspClient`**

**File to Edit**: `Source/LspClient/SpectrumLspClient.h`

**Action**: Add a `QTcpSocket` member and a helper function to find a free port.

```cpp
// In Source/LspClient/SpectrumLspClient.h
#include <QTcpSocket> // Add this include

class SpectrumLspClient : public QObject {
    // ...
private:
    // ... existing members
    std::unique_ptr<QTcpSocket> m_socket; // New member
    
    int findAvailablePort(); // New helper function
    // ...
};```

#### **Task 2.2: Implement the Asynchronous Socket Connection Logic**

**File to Edit**: `Source/LspClient/SpectrumLspClient.cpp`

**Action**: Refactor the `start()` and related methods to use sockets.

```cpp
// In Source/LspClient/SpectrumLspClient.cpp
#include <QTcpServer> // Add for findAvailablePort

int SpectrumLspClient::findAvailablePort() {
    QTcpServer server;
    server.listen(QHostAddress::LocalHost, 0); // 0 tells the OS to pick a free port
    return server.serverPort();
}

bool SpectrumLspClient::start() {
    QMutexLocker locker(&m_stateMutex);
    if (m_connectionState != ConnectionState::Disconnected) return false;
    if (!m_process || m_alsServerPath.isEmpty()) return false;

    setConnectionState(ConnectionState::Connecting);

    int port = findAvailablePort();
    if (port <= 0) {
        qCritical() << "SpectrumLspClient: Could not find an available port.";
        setConnectionState(ConnectionState::Disconnected);
        return false;
    }
    
    qDebug() << "SpectrumLspClient: Found available port:" << port;

    QStringList arguments;
    arguments << "--socket" << QString::number(port);

    // The LspProcess start is already asynchronous
    if (!m_process->start(m_alsServerPath, arguments)) {
        qCritical() << "SpectrumLspClient: Failed to initiate server process start.";
        setConnectionState(ConnectionState::Disconnected);
        return false;
    }

    // Now, instead of waiting for the process, we wait for the socket connection.
    m_socket = std::make_unique<QTcpSocket>(this);
    
    // Connect socket signals to handle connection, errors, and data
    connect(m_socket.get(), &QTcpSocket::connected, this, &SpectrumLspClient::onSocketConnected);
    connect(m_socket.get(), &QTcpSocket::errorOccurred, this, &SpectrumLspClient::onSocketError);
    // We will connect readyRead to the protocol later
    
    qDebug() << "SpectrumLspClient: Attempting to connect to server on port" << port;
    m_socket->connectToHost("127.0.0.1", port);
    m_connectionTimer->start(); // Start timeout for the socket connection

    return true;
}

// Create new slots to handle socket events
private slots:
void SpectrumLspClient::onSocketConnected() {
    m_connectionTimer->stop();
    qDebug() << "SpectrumLspClient: Socket connected successfully. Initializing protocol.";
    
    // The protocol now communicates over the socket, not the process's stdio
    m_protocol->initialize(m_socket.get()); // Requires LspProtocol to be adapted
    
    setConnectionState(ConnectionState::Initializing);
    m_protocol->sendInitialize(m_workspaceRoot);
}

void SpectrumLspClient::onSocketError(QAbstractSocket::SocketError socketError) {
    m_connectionTimer->stop();
    qWarning() << "SpectrumLspClient: Socket connection error:" << socketError << m_socket->errorString();
    emit errorOccurred("Failed to connect to language server: " + m_socket->errorString());
    stop(); // Gracefully stop everything
}
```

#### **Task 2.3: Adapt `LspProtocol` to Use `QTcpSocket`**

**File to Edit**: `Source/LspClient/LspProtocol.cpp` and `.h`

**Action**: Change the protocol class to accept a `QTcpSocket` instead of an `LspProcess`.

```cpp
// In LspProtocol.h
class LspProtocol : public QObject {
public:
    void initialize(QTcpSocket* socket); // Changed from LspProcess*
private:
    QTcpSocket* m_socket = nullptr; // Changed from LspProcess*
    //...
};

// In LspProtocol.cpp
void LspProtocol::initialize(QTcpSocket* socket) {
    m_socket = socket;
    connect(m_socket, &QTcpSocket::readyRead, this, &LspProtocol::onDataReceived);
    // ... (reset other state variables as before)
}

void LspProtocol::onDataReceived() {
    if (!m_socket) return;
    QByteArray data = m_socket->readAll(); // Read from socket instead of process
    if (!data.isEmpty()) {
        processReceivedData(data);
    }
}

void LspProtocol::sendMessage(const QJsonObject& message) {
    // ... (logic to create header and JSON data is the same)
    if (!m_socket || !m_socket->isOpen()) {
        qWarning() << "LspProtocol: Cannot send message - socket not open";
        return;
    }
    m_socket->write(fullMessage); // Write to socket instead of process
}
```

---

### **Phase 3: Final Integration and Cleanup**

1.  **Full Rebuild**: Perform a clean rebuild of both the `als` and `Spectrum` projects.
2.  **Manual Copy**: Manually copy the `alif-language-server` executable to the `Spectrum` build directory one last time.
3.  **Run and Verify**: Launch `Spectrum` and activate the LSP. Check the debug logs. You should see the port being selected, the server starting, the client connecting, and the `initialize` handshake completing. The editor will not hang.
4.  **Remove Obsolete Code**: Once confirmed working, you can remove the old stdio-related logic from the server and client to simplify the codebase.

This socket-based approach is fundamentally more stable and completely sidesteps the issues with stdio buffering and deadlocks, which is the problem you are currently facing.