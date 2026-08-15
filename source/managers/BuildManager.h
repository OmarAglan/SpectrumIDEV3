#pragma once

#include "ProcessWorker.h"
#include "TakweenProtocol.h"
#include <QObject>
#include <QPointer>
#include <QProcess>
#include <QThread>

class QalamConsole;

class BuildManager : public QObject {
    Q_OBJECT

public:
    enum class CompilerExitClass {
        Success,
        SourceError,
        InvalidInvocation,
        Unsupported,
        ToolchainError,
        InternalError,
        Cancelled,
        ProcessFailure,
        Unknown
    };
    Q_ENUM(CompilerExitClass)

    explicit BuildManager(QObject *parent = nullptr);
    ~BuildManager();

    /// Run a Baa source file. Outputs go to the given console.
    void runBaa(const QString &filePath, QalamConsole *console);

    /// Run a supported Takween project command for the project owning filePath.
    bool runTakweenCommand(const QString &filePath,
                           const QString &command,
                           QalamConsole *console,
                           const QString &targetName = QString());

    /// Build argv for the supported Takween project commands, or an empty list.
    static QStringList takweenCommandArguments(const QString &command,
                                               const QString &targetName = QString());

    /// Ask Takween for the authoritative target index; Qalam never parses the manifest.
    QVector<TakweenTarget> discoverTakweenTargets(const QString &filePath,
                                                  QString *error = nullptr) const;

    /// Filter target capabilities for a build/run/test operation.
    static QVector<TakweenTarget> selectableTakweenTargets(
        const QVector<TakweenTarget> &targets,
        const QString &command);

    /// Classify compiler-cli-v1 codes without inspecting human-readable output.
    static CompilerExitClass classifyCompilerExitCode(int exitCode);

    /// Stable diagnostic identifier for a process/compiler exit code.
    static QString compilerExitCodeId(int exitCode);

    /// Arabic, operation-aware fallback used when structured diagnostics are empty.
    static QString compilerExitSummary(int exitCode, const QString &operation);

    /// Find the nearest Takween v0/v1 project root containing مشروع.تكوين.
    static QString findTakweenProjectRoot(const QString &filePath);

    /// Resolve the Takween executable shared by builds and Baa-LSP project discovery.
    static QString resolveTakweenProgram();

    /// Stop the currently running build process, if any.
    void stop();

    /// Whether a build is currently running
    bool isRunning() const;

signals:
    /// Emitted when a build starts
    void buildStarted();
    /// Emitted when a build finishes with the given exit code
    void buildFinished(int exitCode);
    /// Raw output chunks from the compiler/process, for diagnostics parsing.
    void outputChunk(const QString &text);
    /// Completion event with an explicit operation and unmodified process exit code.
    void toolingFinished(const QString &operation, int exitCode);
    /// Validated takween-build-events-v1 record.
    void takweenEventReady(const TakweenBuildEvent &event);
    /// Arabic progress text derived only from the structured event contract.
    void toolingProgress(const QString &text);
    /// A malformed, out-of-order, or incomplete event stream was observed.
    void toolingProtocolError(const QString &message);

private:
    /// Resolve the compiler path from settings or default locations
    QString resolveCompilerPath() const;

    /// Clean up existing thread/worker safely
    void cleanupBuild();
    void startProcess(const QString &program,
                      const QStringList &arguments,
                      const QString &workingDirectory,
                      const QString &contextPath,
                      const QString &operation,
                      const QString &heading,
                      QalamConsole *console,
                      bool usesTakweenEvents = false,
                      const QString &followUpProgram = QString(),
                      const QStringList &followUpArguments = {},
                      const QString &followUpWorkingDirectory = QString(),
                      const QString &followUpHeading = QString());

    QPointer<ProcessWorker> m_worker;
    QPointer<QThread> m_buildThread;
    qint64 m_lastEventSequence{};
    bool m_terminalEventSeen{};
    bool m_eventProtocolFailed{};
    bool m_cancelRequested{};
    int m_terminalEventExitCode{};
};
