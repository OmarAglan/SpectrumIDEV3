#include "ProjectSearchService.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMetaObject>
#include <QRegularExpression>
#include <QSet>
#include <QStringDecoder>
#include <QTimer>

#include <algorithm>
#include <functional>

namespace {

struct SourceDocument
{
    QString text;
    QByteArray originalBytes;
    bool hasUtf8Bom{};
    int revision{-1};
};

QString normalizedPath(const QString &filePath)
{
    QString path = QDir::cleanPath(QFileInfo(filePath).absoluteFilePath());
#ifdef Q_OS_WIN
    path = path.toCaseFolded();
#endif
    return path;
}

bool createExpression(const ProjectSearchRequest &request,
                      QRegularExpression *expression,
                      QString *error)
{
    if (error) error->clear();
    if (request.query.isEmpty()) {
        if (error) *error = QStringLiteral("عبارة البحث فارغة.");
        return false;
    }

    QRegularExpression::PatternOptions options =
        QRegularExpression::UseUnicodePropertiesOption;
    if (not request.caseSensitive)
        options |= QRegularExpression::CaseInsensitiveOption;

    QString pattern = request.regularExpression
        ? request.query
        : QRegularExpression::escape(request.query);
    if (request.wholeWord) {
        pattern = QStringLiteral(
            "(?<![\\p{L}\\p{M}\\p{N}_])(?:%1)(?![\\p{L}\\p{M}\\p{N}_])")
                      .arg(pattern);
    }

    *expression = QRegularExpression(pattern, options);
    if (not expression->isValid()) {
        if (error) *error = expression->errorString();
        return false;
    }
    return true;
}

bool readSourceDocument(const QString &filePath,
                        const ProjectSearchRequest &request,
                        SourceDocument *document,
                        QString *error)
{
    if (error) error->clear();
    const auto overlay = request.overlays.constFind(normalizedPath(filePath));
    if (overlay != request.overlays.constEnd()) {
        document->text = overlay->text;
        document->revision = overlay->revision;
        return true;
    }

    QFile file(filePath);
    if (not file.open(QIODevice::ReadOnly)) {
        if (error) *error = file.errorString();
        return false;
    }

    document->originalBytes = file.readAll();
    document->hasUtf8Bom = document->originalBytes.startsWith("\xEF\xBB\xBF");
    const QByteArray body = document->hasUtf8Bom
        ? document->originalBytes.mid(3)
        : document->originalBytes;
    QStringDecoder decoder(QStringDecoder::Utf8);
    document->text = decoder.decode(body);
    if (decoder.hasError()) {
        if (error) *error = QStringLiteral("الملف ليس نص UTF-8 صالحًا.");
        return false;
    }
    return true;
}

template <typename Callback>
bool visitLines(const QString &text, Callback callback)
{
    qsizetype lineStart = 0;
    int lineNumber = 0;
    while (lineStart <= text.size()) {
        const qsizetype newline = text.indexOf(QLatin1Char('\n'), lineStart);
        qsizetype lineEnd = newline < 0 ? text.size() : newline;
        if (lineEnd > lineStart and text.at(lineEnd - 1) == QLatin1Char('\r'))
            --lineEnd;
        if (not callback(
                text.mid(lineStart, lineEnd - lineStart),
                lineNumber,
                lineStart))
            return false;
        if (newline < 0) break;
        lineStart = newline + 1;
        ++lineNumber;
    }
    return true;
}

QString expandReplacement(const QString &replacement,
                          const QRegularExpressionMatch &match)
{
    QString expanded;
    expanded.reserve(replacement.size());
    for (qsizetype index = 0; index < replacement.size();) {
        const QChar marker = replacement.at(index);
        if ((marker == QLatin1Char('$') or marker == QLatin1Char('\\')) and
            index + 1 < replacement.size() and
            replacement.at(index + 1).isDigit()) {
            qsizetype end = index + 1;
            while (end < replacement.size() and replacement.at(end).isDigit())
                ++end;
            bool ok = false;
            const int capture = replacement.mid(index + 1, end - index - 1)
                                    .toInt(&ok);
            if (ok and capture <= match.lastCapturedIndex()) {
                expanded += match.captured(capture);
                index = end;
                continue;
            }
        }
        expanded += marker;
        ++index;
    }
    return expanded;
}

QByteArray encodedReplacement(const ProjectReplacementFile &file,
                              bool hasUtf8Bom)
{
    QByteArray bytes;
    if (hasUtf8Bom) bytes.append("\xEF\xBB\xBF");
    bytes.append(file.updatedText.toUtf8());
    return bytes;
}

ProjectSearchResult executeSearch(
    const ProjectSearchRequest &request,
    int requestId,
    const std::atomic_int &generation,
    const std::function<void(int)> &progress)
{
    ProjectSearchResult result;
    result.requestId = requestId;
    QRegularExpression expression;
    if (not createExpression(request, &expression, &result.error)) return result;

    QSet<QString> matchedFiles;
    for (int fileIndex = 0; fileIndex < request.filePaths.size(); ++fileIndex) {
        if (generation.load() != requestId) return {};
        const QString &filePath = request.filePaths.at(fileIndex);
        SourceDocument document;
        QString readError;
        if (not readSourceDocument(filePath, request, &document, &readError)) {
            ++result.skippedFiles;
            ++result.scannedFiles;
            progress(result.scannedFiles);
            continue;
        }

        const bool completed = visitLines(
            document.text,
            [&](const QString &lineText, int lineNumber, qsizetype) {
                if (generation.load() != requestId) return false;
                QRegularExpressionMatchIterator matches =
                    expression.globalMatch(lineText);
                while (matches.hasNext()) {
                    if (generation.load() != requestId) return false;
                    const QRegularExpressionMatch match = matches.next();
                    if (not match.hasMatch() or match.capturedLength() == 0)
                        continue;
                    if (result.matches.size() >=
                        qMax(1, request.maximumMatches)) {
                        result.truncated = true;
                        return false;
                    }
                    matchedFiles.insert(normalizedPath(filePath));
                    result.matches.push_back({
                        filePath,
                        lineText,
                        match.captured(0),
                        lineNumber,
                        static_cast<int>(match.capturedStart()),
                        static_cast<int>(match.capturedLength())
                    });
                }
                return true;
            });
        ++result.scannedFiles;
        progress(result.scannedFiles);
        if (not completed) break;
    }
    result.fileCount = matchedFiles.size();
    return result;
}

ProjectReplacementPlan executeReplacement(
    const ProjectSearchRequest &request,
    int requestId,
    const std::atomic_int &generation,
    const std::function<void(int)> &progress)
{
    ProjectReplacementPlan plan;
    plan.requestId = requestId;
    plan.query = request.query;
    plan.replacement = request.replacement;
    plan.caseSensitive = request.caseSensitive;
    plan.wholeWord = request.wholeWord;
    plan.regularExpression = request.regularExpression;

    QRegularExpression expression;
    if (not createExpression(request, &expression, &plan.error)) return plan;

    for (int fileIndex = 0; fileIndex < request.filePaths.size(); ++fileIndex) {
        if (generation.load() != requestId) return {};
        const QString &filePath = request.filePaths.at(fileIndex);
        SourceDocument document;
        QString readError;
        if (not readSourceDocument(filePath, request, &document, &readError)) {
            plan.error = QStringLiteral("تعذرت قراءة %1: %2")
                             .arg(filePath, readError);
            plan.files.clear();
            plan.replacementCount = 0;
            return plan;
        }

        struct Replacement {
            qsizetype start{};
            qsizetype length{};
            QString text;
        };
        QVector<Replacement> replacements;
        const bool completed = visitLines(document.text,
            [&](const QString &lineText, int, qsizetype lineStart) {
                if (generation.load() != requestId) return false;
                QRegularExpressionMatchIterator matches =
                    expression.globalMatch(lineText);
                while (matches.hasNext()) {
                    if (generation.load() != requestId) return false;
                    const QRegularExpressionMatch match = matches.next();
                    if (not match.hasMatch() or match.capturedLength() == 0)
                        continue;
                    replacements.push_back({
                        lineStart + match.capturedStart(),
                        match.capturedLength(),
                        request.regularExpression
                            ? expandReplacement(request.replacement, match)
                            : request.replacement
                    });
                    if (plan.replacementCount + replacements.size() >
                        qMax(1, request.maximumMatches)) {
                        plan.error = QStringLiteral(
                            "تجاوز الاستبدال الحد الآمن البالغ %1 نتيجة.")
                                         .arg(request.maximumMatches);
                        return false;
                    }
                }
                return true;
            });
        if (generation.load() != requestId) return {};
        if (not plan.error.isEmpty()) {
            plan.files.clear();
            plan.replacementCount = 0;
            return plan;
        }
        if (not completed) return plan;

        if (not replacements.isEmpty()) {
            ProjectReplacementFile replacementFile;
            replacementFile.filePath = filePath;
            replacementFile.originalText = document.text;
            replacementFile.updatedText = document.text;
            replacementFile.originalBytes = document.originalBytes;
            replacementFile.sourceRevision = document.revision;
            replacementFile.replacementCount = replacements.size();
            for (auto it = replacements.crbegin(); it != replacements.crend(); ++it) {
                replacementFile.updatedText.replace(
                    it->start, it->length, it->text);
            }
            replacementFile.updatedBytes = encodedReplacement(
                replacementFile, document.hasUtf8Bom);
            plan.replacementCount += replacements.size();
            plan.files.push_back(std::move(replacementFile));
        }
        ++plan.scannedFiles;
        progress(plan.scannedFiles);
    }
    return plan;
}

}

ProjectSearchService::ProjectSearchService(QObject *parent)
    : QObject(parent)
{
    m_threadPool.setMaxThreadCount(1);
    m_threadPool.setExpiryTimeout(-1);
}

ProjectSearchService::~ProjectSearchService()
{
    cancel();
    m_threadPool.waitForDone();
}

int ProjectSearchService::search(const ProjectSearchRequest &request)
{
    const int requestId = ++m_generation;
    m_threadPool.clear();
    emit searchStarted(requestId, request.filePaths.size());

    const QString key = cacheKey(request);
    if (not key.isEmpty() and key == m_cachedKey) {
        ProjectSearchResult cached = m_cachedResult;
        cached.requestId = requestId;
        QTimer::singleShot(0, this, [this, requestId, cached]() {
            if (m_generation.load() == requestId) emit searchFinished(cached);
        });
        return requestId;
    }

    m_threadPool.start([this, request, requestId, key]() {
        const int totalFiles = request.filePaths.size();
        ProjectSearchResult result = executeSearch(
            request,
            requestId,
            m_generation,
            [this, requestId, totalFiles](int scannedFiles) {
                if (m_generation.load() != requestId) return;
                if (scannedFiles != totalFiles and scannedFiles % 16 != 0)
                    return;
                QMetaObject::invokeMethod(
                    this,
                    [this, requestId, scannedFiles, totalFiles]() {
                        if (m_generation.load() == requestId)
                            emit searchProgress(
                                requestId, scannedFiles, totalFiles);
                    },
                    Qt::QueuedConnection);
            });
        if (m_generation.load() != requestId) return;
        QMetaObject::invokeMethod(
            this,
            [this, requestId, key, result]() {
                if (m_generation.load() != requestId) return;
                if (result.error.isEmpty()) {
                    m_cachedKey = key;
                    m_cachedResult = result;
                }
                emit searchFinished(result);
            },
            Qt::QueuedConnection);
    });
    return requestId;
}

int ProjectSearchService::prepareReplacement(
    const ProjectSearchRequest &request)
{
    const int requestId = ++m_generation;
    m_threadPool.clear();
    emit replacementStarted(requestId, request.filePaths.size());
    m_threadPool.start([this, request, requestId]() {
        const int totalFiles = request.filePaths.size();
        ProjectReplacementPlan plan = executeReplacement(
            request,
            requestId,
            m_generation,
            [this, requestId, totalFiles](int scannedFiles) {
                if (m_generation.load() != requestId) return;
                if (scannedFiles != totalFiles and scannedFiles % 16 != 0)
                    return;
                QMetaObject::invokeMethod(
                    this,
                    [this, requestId, scannedFiles, totalFiles]() {
                        if (m_generation.load() == requestId)
                            emit replacementProgress(
                                requestId, scannedFiles, totalFiles);
                    },
                    Qt::QueuedConnection);
            });
        if (m_generation.load() != requestId) return;
        QMetaObject::invokeMethod(
            this,
            [this, requestId, plan]() {
                if (m_generation.load() == requestId)
                    emit replacementPrepared(plan);
            },
            Qt::QueuedConnection);
    });
    return requestId;
}

void ProjectSearchService::cancel()
{
    ++m_generation;
    m_threadPool.clear();
}

void ProjectSearchService::invalidateCache()
{
    m_cachedKey.clear();
    m_cachedResult = {};
}

QString ProjectSearchService::cacheKey(
    const ProjectSearchRequest &request) const
{
    QCryptographicHash hash(QCryptographicHash::Sha256);
    auto add = [&hash](const QByteArray &value) {
        hash.addData(value);
        hash.addData(QByteArray(1, '\0'));
    };
    add(request.query.toUtf8());
    add(QByteArray::number(request.caseSensitive));
    add(QByteArray::number(request.wholeWord));
    add(QByteArray::number(request.regularExpression));
    add(QByteArray::number(request.maximumMatches));
    for (const QString &filePath : request.filePaths) {
        const QString key = normalizedPath(filePath);
        add(key.toUtf8());
        const auto overlay = request.overlays.constFind(key);
        if (overlay != request.overlays.constEnd()) {
            add(QByteArray::number(overlay->revision));
            add(QCryptographicHash::hash(
                overlay->text.toUtf8(), QCryptographicHash::Sha256));
            continue;
        }
        const QFileInfo info(filePath);
        add(QByteArray::number(info.size()));
        add(QByteArray::number(info.lastModified().toMSecsSinceEpoch()));
    }
    return QString::fromLatin1(hash.result().toHex());
}
