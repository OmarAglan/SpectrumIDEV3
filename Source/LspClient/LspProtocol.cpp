#include "LspProtocol.h"
#include "LspProcess.h"
#include <QDebug>
#include <QJsonArray>
#include <QMutexLocker>
#include <QCoreApplication>

LspProtocol::LspProtocol(QObject* parent)
    : QObject(parent)
    , m_process(nullptr)
    , m_requestIdCounter(1)
    , m_initialized(false)
{
    qDebug() << "LspProtocol: Initializing JSON-RPC protocol handler";
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
    
    m_buffer.clear();
    m_requestIdCounter = 1;
    m_initialized = true;
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

void LspProtocol::onDataReceived()
{
    if (!m_process) {
        return;
    }
    
    QByteArray data = m_process->readAllStandardOutput();
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
