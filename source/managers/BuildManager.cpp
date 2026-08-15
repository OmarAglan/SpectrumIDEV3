#include "BuildManager.h"
#include "QalamConsole.h"
#include "Constants.h"

#include <QSettings>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QStandardPaths>
#include <QMetaObject>
#include <QPointer>
#include <QUuid>

namespace {

QString standaloneRunExecutablePath(const QString &sourcePath)
{
    QString cacheRoot = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (cacheRoot.isEmpty()) {
        cacheRoot = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    }
    if (cacheRoot.isEmpty()) cacheRoot = QDir::tempPath();

    QDir runDirectory(QDir(cacheRoot).filePath(QStringLiteral("تشغيل")));
    if (not runDirectory.exists() and not QDir().mkpath(runDirectory.absolutePath())) {
        return QString();
    }

    QString identity = QFileInfo(sourcePath).canonicalFilePath();
    if (identity.isEmpty()) identity = QFileInfo(sourcePath).absoluteFilePath();
    const QString digest = QString::fromLatin1(
        QCryptographicHash::hash(identity.toUtf8(), QCryptographicHash::Sha256)
            .toHex()
            .left(16));
#if defined(Q_OS_WIN)
    const QString executableName = QStringLiteral("برنامج-%1.exe").arg(digest);
#else
    const QString executableName = QStringLiteral("برنامج-%1").arg(digest);
#endif
    return runDirectory.filePath(executableName);
}

}

BuildManager::BuildManager(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<TakweenBuildEvent>();
    qRegisterMetaType<QVector<TakweenTarget>>();
}

BuildManager::~BuildManager()
{
    cleanupBuild();
}

bool BuildManager::isRunning() const
{
    return m_buildThread and m_buildThread->isRunning();
}

QString BuildManager::resolveCompilerPath() const
{
    QSettings settings(Constants::OrgName, Constants::AppName);
    const QString configuredProgram = settings.value(Constants::SettingsKeyCompilerPath).toString().trimmed();

    if (!configuredProgram.isEmpty()) {
        return configuredProgram;
    }

    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
#if defined(Q_OS_WIN)
        QDir(appDir).filePath("baa/baa.exe"),
        QDir(appDir).filePath("baa.exe"),
        QStandardPaths::findExecutable("baa.exe"),
        QStandardPaths::findExecutable("baa")
#else
        QDir(appDir).filePath("baa/baa"),
        QDir(appDir).filePath("baa"),
        QStandardPaths::findExecutable("baa")
#endif
    };

    for (const QString &candidate : candidates) {
        if (!candidate.isEmpty() and QFileInfo(candidate).isExecutable()) {
            return candidate;
        }
    }

#if defined(Q_OS_WIN)
    return QDir(appDir).filePath("baa/baa.exe");
#else
    return QDir(appDir).filePath("baa/baa");
#endif
}

QString BuildManager::resolveTakweenProgram()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
#if defined(Q_OS_WIN)
        QDir(appDir).filePath("takween/تكوين.exe"),
        QDir(appDir).filePath("takween/takween.exe"),
        QDir(appDir).filePath("تكوين.exe"),
        QDir(appDir).filePath("takween.exe"),
        QStandardPaths::findExecutable("تكوين.exe"),
        QStandardPaths::findExecutable("takween.exe"),
        QStandardPaths::findExecutable("takween")
#else
        QDir(appDir).filePath("takween/تكوين"),
        QDir(appDir).filePath("takween/takween"),
        QStandardPaths::findExecutable("تكوين"),
        QStandardPaths::findExecutable("takween")
#endif
    };

    for (const QString &candidate : candidates) {
        if (!candidate.isEmpty() and QFileInfo(candidate).isExecutable()) {
            return candidate;
        }
    }
    return QString();
}

QStringList BuildManager::takweenCommandArguments(const QString &command,
                                                   const QString &targetName)
{
    const QString normalized = command.trimmed().toLower();
    QString canonical;
    if (normalized == "build") canonical = "بناء";
    else if (normalized == "run") canonical = "تشغيل";
    else if (normalized == "test") canonical = "اختبار";
    else if (normalized == "clean") canonical = "تنظيف";
    else return {};

    QStringList arguments = {canonical};
    if (not targetName.trimmed().isEmpty()) {
        if (normalized == "clean") return {};
        arguments.push_back(targetName.trimmed());
    }
    return arguments;
}

BuildManager::CompilerExitClass BuildManager::classifyCompilerExitCode(int exitCode)
{
    switch (exitCode) {
    case 0: return CompilerExitClass::Success;
    case 1: return CompilerExitClass::SourceError;
    case 2: return CompilerExitClass::InvalidInvocation;
    case 3: return CompilerExitClass::Unsupported;
    case 4: return CompilerExitClass::ToolchainError;
    case 5: return CompilerExitClass::InternalError;
    case -2: return CompilerExitClass::Cancelled;
    default:
        return exitCode < 0 ? CompilerExitClass::ProcessFailure : CompilerExitClass::Unknown;
    }
}

QString BuildManager::compilerExitCodeId(int exitCode)
{
    if (exitCode >= 0 and exitCode <= 5) {
        return QString("CLI_EXIT_%1").arg(exitCode);
    }
    if (exitCode == -2) return "TOOLING_CANCELLED";
    if (exitCode < 0) return "PROCESS_FAILURE";
    return QString("UNKNOWN_EXIT_%1").arg(exitCode);
}

QString BuildManager::compilerExitSummary(int exitCode, const QString &operation)
{
    if (exitCode == -2) return "أُلغيت عملية البناء بطلب المستخدم.";
    const QString normalized = operation.trimmed().toLower();
    const bool mayBeProgramExit = normalized == "run" or normalized == "test";
    if (mayBeProgramExit) {
        return QString("انتهى أمر %1 بكود خروج %2. بعد نجاح البناء قد يكون هذا كود البرنامج الناتج.")
            .arg(normalized, QString::number(exitCode));
    }

    switch (classifyCompilerExitCode(exitCode)) {
    case CompilerExitClass::Success:
        return QString();
    case CompilerExitClass::SourceError:
        return "رفضت الأداة المصدر أو المشروع (كود الخروج 1).";
    case CompilerExitClass::InvalidInvocation:
        return "استدعاء أداة البناء غير صالح (كود الخروج 2).";
    case CompilerExitClass::Unsupported:
        return "الهدف أو الميزة المطلوبة غير مدعومة (كود الخروج 3).";
    case CompilerExitClass::ToolchainError:
        return "فشلت أداة البناء أو backend أو إنشاء المخرجات (كود الخروج 4).";
    case CompilerExitClass::InternalError:
        return "حدث خطأ داخلي في المصرّف أو نظام البناء (كود الخروج 5).";
    case CompilerExitClass::Cancelled:
        return "أُلغيت عملية البناء بطلب المستخدم.";
    case CompilerExitClass::ProcessFailure:
        return "تعذر بدء أداة البناء أو انقطعت العملية قبل اكتمالها.";
    case CompilerExitClass::Unknown:
        return QString("انتهت أداة البناء بكود خروج غير معروف: %1.").arg(exitCode);
    }
    return QString();
}

QString BuildManager::findTakweenProjectRoot(const QString &filePath)
{
    QDir directory = QFileInfo(filePath).isDir()
        ? QDir(filePath)
        : QFileInfo(filePath).absoluteDir();

    while (directory.exists()) {
        if (QFileInfo(directory.filePath("مشروع.تكوين")).isFile()) {
            return QDir::cleanPath(directory.absolutePath());
        }
        if (!directory.cdUp()) break;
    }
    return QString();
}

QVector<TakweenTarget> BuildManager::discoverTakweenTargets(const QString &filePath,
                                                            QString *error) const
{
    const QString projectRoot = findTakweenProjectRoot(filePath);
    const QString takween = resolveTakweenProgram();
    if (projectRoot.isEmpty()) {
        if (error) *error = "لم يُعثر على مشروع.تكوين.";
        return {};
    }
    if (takween.isEmpty()) {
        if (error) *error = "لم يُعثر على برنامج تكوين القابل للتنفيذ.";
        return {};
    }

    QProcess process;
    process.setProgram(takween);
    process.setArguments({"أهداف", "--جسون"});
    process.setWorkingDirectory(projectRoot);
    process.setProcessChannelMode(QProcess::SeparateChannels);
    process.start();
    if (not process.waitForStarted(3000)) {
        if (error) *error = "تعذر بدء تكوين لاكتشاف الأهداف: " + process.errorString();
        return {};
    }
    if (not process.waitForFinished(5000)) {
        process.kill();
        process.waitForFinished(500);
        if (error) *error = "انتهت مهلة اكتشاف أهداف تكوين.";
        return {};
    }
    if (process.exitStatus() != QProcess::NormalExit or process.exitCode() != 0) {
        if (error) {
            const QString detail = QString::fromUtf8(process.readAllStandardError()).trimmed();
            *error = detail.isEmpty()
                ? QString("فشل اكتشاف أهداف تكوين بكود %1.").arg(process.exitCode())
                : detail;
        }
        return {};
    }

    QVector<TakweenTarget> targets;
    QString parseError;
    if (not TakweenProtocol::parseTargets(process.readAllStandardOutput(), &targets, &parseError)) {
        if (error) *error = parseError;
        return {};
    }
    return targets;
}

QVector<TakweenTarget> BuildManager::selectableTakweenTargets(
    const QVector<TakweenTarget> &targets,
    const QString &command)
{
    const QString normalized = command.trimmed().toLower();
    QVector<TakweenTarget> selected;
    for (const TakweenTarget &target : targets) {
        const bool selectable =
            (normalized == "build" and target.buildable) or
            (normalized == "run" and target.runnable) or
            (normalized == "test" and target.test);
        if (selectable) selected.push_back(target);
    }
    return selected;
}

void BuildManager::cleanupBuild()
{
    QThread *thread = m_buildThread.data();
    ProcessWorker *worker = m_worker.data();

    m_worker = nullptr;
    m_buildThread = nullptr;

    if (worker) {
        if (thread and thread->isRunning() and QThread::currentThread() != thread) {
            QMetaObject::invokeMethod(worker, "stop", Qt::BlockingQueuedConnection);
        } else {
            worker->stop();
        }
    }

    if (thread) {
        thread->quit();
        if (!thread->wait(3000)) {
            thread->terminate();
            thread->wait();
        }
    }
}

void BuildManager::stop()
{
    if (isRunning()) m_cancelRequested = true;
    cleanupBuild();
}

void BuildManager::runBaa(const QString &filePath, QalamConsole *console)
{
    if (!console) return;

    QString program = resolveCompilerPath();
    QStringList args = { filePath };
    QString workingDir = QFileInfo(filePath).absolutePath();
    bool usingTakween = false;
    QString followUpProgram;

    const QString projectRoot = findTakweenProjectRoot(filePath);
    if (!projectRoot.isEmpty()) {
        const QString takween = resolveTakweenProgram();
        if (!takween.isEmpty()) {
            program = takween;
            args = takweenCommandArguments("run");
            workingDir = projectRoot;
            usingTakween = true;
        }
    }

    if (not usingTakween) {
        followUpProgram = standaloneRunExecutablePath(filePath);
        if (followUpProgram.isEmpty()) {
            console->clear();
            console->appendPlainTextThreadSafe(
                QStringLiteral("❌ تعذر إنشاء مجلد تشغيل مؤقت لبرنامج باء.\n"));
            emit toolingFinished(QStringLiteral("run"), -1);
            return;
        }
        QFile::remove(followUpProgram);
        args << QStringLiteral("-o") << followUpProgram;
    }

    startProcess(program,
                 args,
                 workingDir,
                 filePath,
                 QStringLiteral("run"),
                 usingTakween ? "🚀 تشغيل مشروع تكوين...\n" : "🚀 بدء تشغيل ملف باء...\n",
                 console,
                 usingTakween,
                 followUpProgram,
                 {},
                 workingDir,
                 usingTakween ? QString() : QStringLiteral("\n▶ مخرجات البرنامج:\n"));
}

bool BuildManager::runTakweenCommand(const QString &filePath,
                                     const QString &command,
                                     QalamConsole *console,
                                     const QString &targetName)
{
    if (!console) return false;

    const QStringList arguments = takweenCommandArguments(command, targetName);
    const QString projectRoot = findTakweenProjectRoot(filePath);
    const QString takween = resolveTakweenProgram();
    if (arguments.isEmpty() or projectRoot.isEmpty() or takween.isEmpty()) return false;

    const QString normalized = command.trimmed().toLower();
    QString heading = "🚀 تنفيذ أمر مشروع تكوين...\n";
    if (normalized == "build") heading = "🛠️ بناء مشروع تكوين...\n";
    else if (normalized == "run") heading = "🚀 تشغيل مشروع تكوين...\n";
    else if (normalized == "test") heading = "🧪 اختبار مشروع تكوين...\n";
    else if (normalized == "clean") heading = "🧹 تنظيف مشروع تكوين...\n";

    startProcess(
        takween, arguments, projectRoot, filePath, normalized, heading, console, true);
    return true;
}

void BuildManager::startProcess(const QString &requestedProgram,
                                const QStringList &arguments,
                                const QString &workingDirectory,
                                const QString &contextPath,
                                const QString &operation,
                                const QString &heading,
                                QalamConsole *console,
                                bool usesTakweenEvents,
                                const QString &followUpProgram,
                                const QStringList &followUpArguments,
                                const QString &followUpWorkingDirectory,
                                const QString &followUpHeading)
{
    if (!console) return;
    QString program = requestedProgram;

    if (!QFileInfo(program).isExecutable()) {
        const QString pathProgram = QStandardPaths::findExecutable(program);
        if (!pathProgram.isEmpty()) {
            program = pathProgram;
        }
    }

    if (!QFileInfo(program).isExecutable()) {
        console->clear();
        console->appendPlainTextThreadSafe("❌ خطأ: لم يتم العثور على مترجم باء!\n");
        console->appendPlainTextThreadSafe("المسار المتوقع: " + program + "\n");
        console->appendPlainTextThreadSafe("يمكنك وضع المترجم بجانب التطبيق داخل baa/ أو ضبط مساره من الإعدادات.\n");

#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
        console->appendPlainTextThreadSafe("تأكد من أن ملف baa لديه صلاحية التنفيذ (chmod +x).\n");
#endif
        emit toolingFinished(operation, -1);
        return;
    }

    // Safely clean up existing thread/worker before creating new ones.
    // The same console hosts an interactive shell, so stop it while Baa owns stdin/stdout.
    cleanupBuild();
    console->stopCmd();

    console->clear();
    console->appendPlainTextThreadSafe(heading);
    console->appendPlainTextThreadSafe("📄 السياق: " + QFileInfo(contextPath).fileName() + "\n");

    QStringList processArguments = arguments;
    QString eventFilePath;
    if (usesTakweenEvents) {
        QString temporaryRoot = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        if (temporaryRoot.isEmpty()) temporaryRoot = QDir::tempPath();
        eventFilePath = QDir(temporaryRoot).filePath(
            "قلم-أحداث-تكوين-" + QUuid::createUuid().toString(QUuid::WithoutBraces) + ".jsonl");
        QFile::remove(eventFilePath);
        processArguments << "--ملف_أحداث" << eventFilePath;
    }

    m_lastEventSequence = 0;
    m_terminalEventSeen = false;
    m_eventProtocolFailed = false;
    m_terminalEventExitCode = 0;
    m_cancelRequested = false;

    m_worker = new ProcessWorker(
        program,
        processArguments,
        workingDirectory,
        eventFilePath,
        followUpProgram,
        followUpArguments,
        followUpWorkingDirectory,
        followUpHeading);
    m_buildThread = new QThread(this);

    m_worker->moveToThread(m_buildThread);

    connect(m_buildThread, &QThread::started, m_worker, &ProcessWorker::start);

    connect(m_worker, &ProcessWorker::outputReady, this, [this, console](const QString &text) {
        console->appendPlainTextThreadSafe(text);
        emit outputChunk(text);
    });
    connect(m_worker, &ProcessWorker::errorReady, this, [this, console](const QString &text) {
        console->appendPlainTextThreadSafe(text);
        emit outputChunk(text);
    });
    connect(m_worker, &ProcessWorker::eventLineReady, this,
            [this, console, operation](const QByteArray &line) {
                auto reject = [this, console](const QString &message) {
                    if (m_eventProtocolFailed) return;
                    m_eventProtocolFailed = true;
                    const QString diagnostic = "❌ خرق عقد أحداث تكوين: " + message;
                    console->appendPlainTextThreadSafe(diagnostic + "\n");
                    emit toolingProtocolError(diagnostic);
                };

                TakweenBuildEvent event;
                QString error;
                if (not TakweenProtocol::parseBuildEvent(line, &event, &error)) {
                    reject(error);
                    return;
                }
                if (not TakweenProtocol::validateTransition(
                        event, operation, m_lastEventSequence, m_terminalEventSeen, &error)) {
                    reject(error);
                    return;
                }

                m_lastEventSequence = event.sequence;
                if (event.event == "operation_finished") {
                    m_terminalEventSeen = true;
                    m_terminalEventExitCode = event.exitCode;
                }
                emit takweenEventReady(event);
                const QString progress = TakweenProtocol::progressText(event);
                if (not progress.isEmpty()) emit toolingProgress(progress);
            });

    QThread *thread = m_buildThread.data();
    ProcessWorker *worker = m_worker.data();
    QPointer<QalamConsole> safeConsole(console);

    connect(m_worker, &ProcessWorker::finished, this,
            [this, safeConsole, thread, operation, eventFilePath](int code) {
        int effectiveCode = code;
        if (m_cancelRequested or code == -2) {
            effectiveCode = -2;
        } else if (not eventFilePath.isEmpty()) {
            QString completionError;
            if (not TakweenProtocol::validateCompletion(
                    code, false, m_eventProtocolFailed, m_terminalEventSeen,
                    m_terminalEventExitCode, &completionError)) {
                effectiveCode = -1;
                if (not m_eventProtocolFailed) {
                    const QString message = "❌ خرق عقد أحداث تكوين: " + completionError;
                    if (safeConsole) safeConsole->appendPlainTextThreadSafe(message + "\n");
                    emit toolingProtocolError(message);
                }
            }
        }
        if (safeConsole) {
            QString result;
            if (effectiveCode == -2) {
                result = "\n──────────────────────────────\n⏹ أُلغيت العملية.\n";
            } else if (effectiveCode == 0) {
                result = "\n──────────────────────────────\n✅ اكتمل الأمر بنجاح.\n";
            } else {
                result = "\n──────────────────────────────\n❌ فشل الأمر (Exit code = "
                    + QString::number(effectiveCode) + ")\n";
            }
            safeConsole->appendPlainTextThreadSafe(result);
            safeConsole->startCmd();
        }
        if (thread) {
            thread->quit();
        }
        m_cancelRequested = false;
        emit buildFinished(effectiveCode);
        emit toolingFinished(operation, effectiveCode);
    });

    // Cleanup logic: ensure pointers are cleared after the worker thread finishes.
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    connect(thread, &QThread::finished, this, [this, thread, eventFilePath]() {
        if (not eventFilePath.isEmpty()) QFile::remove(eventFilePath);
        if (m_buildThread == thread) {
            m_buildThread = nullptr;
        }
        m_worker = nullptr;
    });

    connect(console, &QalamConsole::commandEntered,
            m_worker, &ProcessWorker::sendInput);

    emit buildStarted();
    m_buildThread->start();
}
