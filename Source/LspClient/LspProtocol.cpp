#include "LspProtocol.h"
#include "LspProcess.h"
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMutexLocker>
#include <QCoreApplication>
#include <QDateTime>
#include <queue>

LspProtocol::LspProtocol(QObject* parent)
    : QObject(parent)
    , m_process(nullptr)
    , m_socket(nullptr)
    , m_requestIdCounter(1)
    , m_initialized(false)
    , m_ready(false)
    , m_defaultTimeoutMs(DEFAULT_TIMEOUT_MS)
    , m_queueTimer(new QTimer(this))
{
    qDebug() << "LspProtocol: Initializing enhanced JSON-RPC protocol handler";

    // Setup message queue processing timer
    m_queueTimer->setInterval(QUEUE_PROCESS_INTERVAL_MS);
    connect(m_queueTimer, &QTimer::timeout, this, &LspProtocol::processMessageQueue);
    m_queueTimer->start();
}

LspProtocol::~LspProtocol()
{
    qDebug() << "LspProtocol: Destructor called";
    shutdown();
}

void LspProtocol::initialize(LspProcess* process)
{
    qDebug() << "LspProtocol: Initializing with process";

    if (m_process) {
        m_process->disconnect(this);
    }

    m_process = process;

    if (m_process) {
        connect(m_process, &LspProcess::readyReadStandardOutput,
                this, &LspProtocol::onDataReceived);
    }

    // Reset state
    m_buffer.clear();
    m_requestIdCounter = 1;
    m_initialized = true;
    m_ready = false;

    // Clear any pending requests
    {
        QMutexLocker locker(&m_requestMutex);
        for (auto it = m_pendingRequests.begin(); it != m_pendingRequests.end(); ++it) {
            delete it.value()->timeoutTimer;
            delete it.value();
        }
        m_pendingRequests.clear();
    }

    // Clear message queue
    while (!m_messageQueue.empty()) {
        m_messageQueue.pop();
    }
}

void LspProtocol::initialize(QTcpSocket* socket)
{
    qDebug() << "LspProtocol: Initializing with socket";

    // Disconnect from previous process if any
    if (m_process) {
        m_process->disconnect(this);
        m_process = nullptr;
    }

    // Disconnect from previous socket if any
    if (m_socket) {
        m_socket->disconnect(this);
    }

    m_socket = socket;

    if (m_socket) {
        connect(m_socket, &QTcpSocket::readyRead,
                this, &LspProtocol::onDataReceived);
    }

    // Reset state
    m_buffer.clear();
    m_requestIdCounter = 1;
    m_initialized = true;
    m_ready = false;

    // Clear any pending requests
    {
        QMutexLocker locker(&m_requestMutex);
        for (auto it = m_pendingRequests.begin(); it != m_pendingRequests.end(); ++it) {
            delete it.value()->timeoutTimer;
            delete it.value();
        }
        m_pendingRequests.clear();
    }

    // Clear message queue
    while (!m_messageQueue.empty()) {
        m_messageQueue.pop();
    }
}

void LspProtocol::sendInitialize(const QString& workspaceRoot)
{
    qDebug() << "LspProtocol: Sending initialize request for workspace:" << workspaceRoot;
    
    QJsonObject params;
    params["processId"] = QCoreApplication::applicationPid();
    params["rootUri"] = QString("file://%1").arg(workspaceRoot);
    params["rootPath"] = workspaceRoot;
    
    // Client capabilities
    QJsonObject clientCapabilities;
    QJsonObject textDocument;
    QJsonObject completion;
    completion["completionItem"] = QJsonObject{
        {"snippetSupport", true},
        {"commitCharactersSupport", true}
    };
    textDocument["completion"] = completion;
    textDocument["hover"] = QJsonObject{{"contentFormat", QJsonArray{"markdown", "plaintext"}}};
    textDocument["definition"] = QJsonObject{{"linkSupport", true}};
    clientCapabilities["textDocument"] = textDocument;
    
    QJsonObject workspace;
    workspace["workspaceFolders"] = true;
    workspace["configuration"] = true;
    clientCapabilities["workspace"] = workspace;
    
    params["capabilities"] = clientCapabilities;
    
    // Client info
    QJsonObject clientInfo;
    clientInfo["name"] = "SpectrumIDE";
    clientInfo["version"] = "1.0.0";
    params["clientInfo"] = clientInfo;
    
    QJsonObject message;
    message["jsonrpc"] = "2.0";
    message["id"] = nextRequestId();
    message["method"] = "initialize";
    message["params"] = params;
    
    sendMessage(message);
}

void LspProtocol::sendInitialized()
{
    qDebug() << "LspProtocol: Sending initialized notification";

    QJsonObject message;
    message["jsonrpc"] = "2.0";
    message["method"] = "initialized";
    message["params"] = QJsonObject();

    sendMessage(message);
    m_ready = true;  // Mark as ready after initialized
}

void LspProtocol::sendShutdown()
{
    qDebug() << "LspProtocol: Sending shutdown request";

    QJsonObject message;
    message["jsonrpc"] = "2.0";
    message["id"] = nextRequestId();
    message["method"] = "shutdown";
    message["params"] = QJsonObject();

    sendMessage(message);
}

void LspProtocol::sendTextDocumentDidOpen(const QString& uri, const QString& languageId,
                                         int version, const QString& text)
{
    qDebug() << "LspProtocol: Sending textDocument/didOpen for" << uri;

    QJsonObject textDocument;
    textDocument["uri"] = uri;
    textDocument["languageId"] = languageId;
    textDocument["version"] = version;
    textDocument["text"] = text;

    QJsonObject params;
    params["textDocument"] = textDocument;

    sendNotification("textDocument/didOpen", params, MessagePriority::High);
}

void LspProtocol::sendTextDocumentDidChange(const QString& uri, int version,
                                           const QJsonArray& changes)
{
    qDebug() << "LspProtocol: Sending textDocument/didChange for" << uri;

    QJsonObject versionedTextDocumentIdentifier;
    versionedTextDocumentIdentifier["uri"] = uri;
    versionedTextDocumentIdentifier["version"] = version;

    QJsonObject params;
    params["textDocument"] = versionedTextDocumentIdentifier;
    params["contentChanges"] = changes;

    sendNotification("textDocument/didChange", params, MessagePriority::High);
}

void LspProtocol::sendTextDocumentDidClose(const QString& uri)
{
    qDebug() << "LspProtocol: Sending textDocument/didClose for" << uri;

    QJsonObject params;
    params["textDocument"] = createTextDocumentIdentifier(uri);

    sendNotification("textDocument/didClose", params, MessagePriority::Normal);
}

void LspProtocol::sendPing()
{
    // LSP doesn't have a standard ping, but we can send a workspace/configuration request
    // as a health check if the server supports it
    qDebug() << "LspProtocol: Sending health check ping";
    
    QJsonObject message;
    message["jsonrpc"] = "2.0";
    message["id"] = nextRequestId();
    message["method"] = "workspace/configuration";
    message["params"] = QJsonObject{
        {"items", QJsonArray{QJsonObject{{"section", "alif"}}}}
    };
    
    sendMessage(message);
}

int LspProtocol::sendTextDocumentCompletion(const QString& uri, int line, int character,
                                           std::function<void(const QJsonObject&)> callback,
                                           std::function<void(const QString&)> errorCallback)
{
    qDebug() << "LspProtocol: Sending textDocument/completion for" << uri
             << "at" << line << ":" << character;

    QJsonObject params;
    params["textDocument"] = createTextDocumentIdentifier(uri);
    params["position"] = createPosition(line, character);

    return sendRequest("textDocument/completion", params, MessagePriority::High,
                      callback, errorCallback);
}

int LspProtocol::sendTextDocumentHover(const QString& uri, int line, int character,
                                      std::function<void(const QJsonObject&)> callback,
                                      std::function<void(const QString&)> errorCallback)
{
    qDebug() << "LspProtocol: Sending textDocument/hover for" << uri
             << "at" << line << ":" << character;

    QJsonObject params;
    params["textDocument"] = createTextDocumentIdentifier(uri);
    params["position"] = createPosition(line, character);

    return sendRequest("textDocument/hover", params, MessagePriority::Normal,
                      callback, errorCallback);
}

int LspProtocol::sendTextDocumentDefinition(const QString& uri, int line, int character,
                                           std::function<void(const QJsonObject&)> callback,
                                           std::function<void(const QString&)> errorCallback)
{
    qDebug() << "LspProtocol: Sending textDocument/definition for" << uri
             << "at" << line << ":" << character;

    QJsonObject params;
    params["textDocument"] = createTextDocumentIdentifier(uri);
    params["position"] = createPosition(line, character);

    return sendRequest("textDocument/definition", params, MessagePriority::Normal,
                      callback, errorCallback);
}

void LspProtocol::shutdown()
{
    qDebug() << "LspProtocol: Shutting down protocol";
    
    if (m_process) {
        m_process->disconnect(this);
        m_process = nullptr;
    }
    
    m_buffer.clear();
    m_initialized = false;
}

int LspProtocol::sendRequest(const QString& method, const QJsonObject& params,
                            MessagePriority priority,
                            std::function<void(const QJsonObject&)> callback,
                            std::function<void(const QString&)> errorCallback)
{
    int requestId = nextRequestId();

    QJsonObject message;
    message["jsonrpc"] = "2.0";
    message["id"] = requestId;
    message["method"] = method;
    message["params"] = params;

    // Create pending request
    PendingRequest* pendingRequest = new PendingRequest();
    pendingRequest->id = requestId;
    pendingRequest->request = message;
    pendingRequest->successCallback = callback;
    pendingRequest->errorCallback = errorCallback;
    pendingRequest->priority = priority;
    pendingRequest->timestamp = QDateTime::currentMSecsSinceEpoch();

    // Setup timeout timer
    pendingRequest->timeoutTimer = new QTimer();
    pendingRequest->timeoutTimer->setSingleShot(true);
    pendingRequest->timeoutTimer->setInterval(m_defaultTimeoutMs);
    connect(pendingRequest->timeoutTimer, &QTimer::timeout, this, &LspProtocol::onRequestTimeout);

    // Store pending request
    {
        QMutexLocker locker(&m_requestMutex);
        m_pendingRequests[requestId] = pendingRequest;
    }

    // Start timeout timer
    pendingRequest->timeoutTimer->start();

    // Queue message for sending
    queueMessage(message, priority);

    qDebug() << "LspProtocol: Queued request" << requestId << "method:" << method;
    return requestId;
}

void LspProtocol::sendNotification(const QString& method, const QJsonObject& params,
                                  MessagePriority priority)
{
    QJsonObject message;
    message["jsonrpc"] = "2.0";
    message["method"] = method;
    message["params"] = params;

    queueMessage(message, priority);
    qDebug() << "LspProtocol: Queued notification method:" << method;
}

bool LspProtocol::cancelRequest(int requestId)
{
    QMutexLocker locker(&m_requestMutex);

    auto it = m_pendingRequests.find(requestId);
    if (it != m_pendingRequests.end()) {
        PendingRequest* request = it.value();

        // Send cancellation notification
        QJsonObject cancelParams;
        cancelParams["id"] = requestId;
        sendNotification("$/cancelRequest", cancelParams, MessagePriority::High);

        // Clean up pending request
        if (request->errorCallback) {
            request->errorCallback("Request cancelled");
        }
        delete request->timeoutTimer;
        delete request;
        m_pendingRequests.erase(it);

        qDebug() << "LspProtocol: Cancelled request" << requestId;
        return true;
    }

    return false;
}

void LspProtocol::setDefaultTimeout(int timeoutMs)
{
    m_defaultTimeoutMs = timeoutMs;
    qDebug() << "LspProtocol: Default timeout set to" << timeoutMs << "ms";
}

int LspProtocol::getPendingRequestCount() const
{
    QMutexLocker locker(&m_requestMutex);
    return m_pendingRequests.size();
}

bool LspProtocol::isReady() const
{
    bool hasValidChannel = false;
    if (m_socket && m_socket->isOpen()) {
        hasValidChannel = true;
    } else if (m_process && m_process->isRunning()) {
        hasValidChannel = true;
    }

    return m_initialized && m_ready && hasValidChannel;
}

void LspProtocol::onDataReceived()
{
    QByteArray data;

    if (m_socket) {
        data = m_socket->readAll();
    } else if (m_process) {
        data = m_process->readAllStandardOutput();
    } else {
        return;
    }

    if (!data.isEmpty()) {
        processReceivedData(data);
    }
}

void LspProtocol::sendMessage(const QJsonObject& message)
{
    QMutexLocker locker(&m_sendMutex);
    
    if (!m_process || !m_process->isRunning()) {
        qWarning() << "LspProtocol: Cannot send message - process not running";
        return;
    }
    
    QJsonDocument doc(message);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    // LSP uses Content-Length header
    QString header = QString("Content-Length: %1\r\n\r\n").arg(jsonData.length());
    QByteArray headerData = header.toUtf8();
    
    QByteArray fullMessage = headerData + jsonData;
    
    qDebug() << "LspProtocol: Sending message:" << jsonData;
    
    qint64 bytesWritten = m_process->write(fullMessage);
    if (bytesWritten == -1) {
        qCritical() << "LspProtocol: Failed to send message";
        emit errorOccurred("Failed to send message to server");
    }
}

void LspProtocol::processReceivedData(const QByteArray& data)
{
    m_buffer.append(data);
    
    // Process all complete messages in buffer
    while (parseMessage()) {
        // parseMessage() processes one message and returns true if successful
    }
}

bool LspProtocol::parseMessage()
{
    // Look for Content-Length header
    int headerEnd = m_buffer.indexOf("\r\n\r\n");
    if (headerEnd == -1) {
        return false; // Incomplete header
    }
    
    QString header = QString::fromUtf8(m_buffer.left(headerEnd));
    QStringList headerLines = header.split("\r\n");
    
    int contentLength = -1;
    for (const QString& line : headerLines) {
        if (line.startsWith("Content-Length:")) {
            bool ok;
            contentLength = line.mid(15).trimmed().toInt(&ok);
            if (!ok) {
                qWarning() << "LspProtocol: Invalid Content-Length header";
                return false;
            }
            break;
        }
    }
    
    if (contentLength == -1) {
        qWarning() << "LspProtocol: Missing Content-Length header";
        m_buffer.clear(); // Clear invalid data
        return false;
    }
    
    int messageStart = headerEnd + 4;
    if (m_buffer.length() < messageStart + contentLength) {
        return false; // Incomplete message
    }
    
    // Extract message content
    QByteArray messageData = m_buffer.mid(messageStart, contentLength);
    m_buffer.remove(0, messageStart + contentLength);
    
    // Parse JSON
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(messageData, &error);
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "LspProtocol: JSON parse error:" << error.errorString();
        return true; // Continue processing buffer
    }
    
    QJsonObject message = doc.object();
    qDebug() << "LspProtocol: Received message:" << messageData;
    
    // Handle different message types
    if (message.contains("id") && message.contains("result")) {
        // Response message
        if (message["method"].toString() == "initialize") {
            emit initializeResponseReceived(message);
        }
    } else if (message.contains("method") && !message.contains("id")) {
        // Notification message
        QString method = message["method"].toString();
        QJsonObject params = message["params"].toObject();
        emit notificationReceived(method, params);
    } else if (message.contains("error")) {
        // Error response
        QJsonObject errorObj = message["error"].toObject();
        QString errorMsg = errorObj["message"].toString();
        qWarning() << "LspProtocol: Server error:" << errorMsg;
        emit errorOccurred(errorMsg);
    }
    
    return true; // Successfully processed one message
}

int LspProtocol::nextRequestId()
{
    return m_requestIdCounter++;
}

QJsonObject LspProtocol::createPosition(int line, int character)
{
    QJsonObject position;
    position["line"] = line;
    position["character"] = character;
    return position;
}

QJsonObject LspProtocol::createTextDocumentIdentifier(const QString& uri)
{
    QJsonObject textDocument;
    textDocument["uri"] = uri;
    return textDocument;
}

void LspProtocol::onRequestTimeout()
{
    QTimer* timer = qobject_cast<QTimer*>(sender());
    if (!timer) return;

    QMutexLocker locker(&m_requestMutex);

    // Find the request associated with this timer
    for (auto it = m_pendingRequests.begin(); it != m_pendingRequests.end(); ++it) {
        PendingRequest* request = it.value();
        if (request->timeoutTimer == timer) {
            int requestId = request->id;
            QString method = request->request["method"].toString();

            qWarning() << "LspProtocol: Request" << requestId << "timed out, method:" << method;

            // Call error callback
            if (request->errorCallback) {
                request->errorCallback("Request timed out");
            }

            // Emit timeout signal
            emit requestTimedOut(requestId, method);

            // Clean up
            delete request->timeoutTimer;
            delete request;
            m_pendingRequests.erase(it);
            break;
        }
    }
}

void LspProtocol::processMessageQueue()
{
    if (m_messageQueue.empty() || !isReady()) {
        return;
    }

    // Process one message from the queue
    QueuedMessage queuedMsg = m_messageQueue.top();
    m_messageQueue.pop();

    sendMessageImmediate(queuedMsg.message);
}

void LspProtocol::queueMessage(const QJsonObject& message, MessagePriority priority)
{
    QueuedMessage queuedMsg;
    queuedMsg.message = message;
    queuedMsg.priority = priority;
    queuedMsg.timestamp = QDateTime::currentMSecsSinceEpoch();

    m_messageQueue.push(queuedMsg);
}

void LspProtocol::sendMessageImmediate(const QJsonObject& message)
{
    QMutexLocker locker(&m_sendMutex);

    // Check if we have a valid communication channel
    bool canSend = false;
    if (m_socket && m_socket->isOpen()) {
        canSend = true;
    } else if (m_process && m_process->isRunning()) {
        canSend = true;
    }

    if (!canSend) {
        qWarning() << "LspProtocol: Cannot send message - no valid communication channel";
        return;
    }

    QJsonDocument doc(message);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);

    // LSP uses Content-Length header
    QString header = QString("Content-Length: %1\r\n\r\n").arg(jsonData.size());
    QByteArray headerData = header.toUtf8();

    QByteArray fullMessage = headerData + jsonData;

    qint64 written = 0;
    if (m_socket) {
        written = m_socket->write(fullMessage);
    } else if (m_process) {
        written = m_process->write(fullMessage);
    }

    if (written != fullMessage.size()) {
        qWarning() << "LspProtocol: Failed to write complete message";
        emit errorOccurred("Failed to send message to server");
    } else {
        qDebug() << "LspProtocol: Sent message:" << jsonData.left(200) << "...";
    }
}
