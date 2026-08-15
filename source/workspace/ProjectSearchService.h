#pragma once

#include <QByteArray>
#include <QHash>
#include <QMetaType>
#include <QObject>
#include <QStringList>
#include <QThreadPool>
#include <QVector>

#include <atomic>

struct ProjectDocumentOverlay {
    QString text;
    int revision{-1};
};

struct ProjectSearchRequest {
    QString rootPath;
    QStringList filePaths;
    QHash<QString, ProjectDocumentOverlay> overlays;
    QString query;
    QString replacement;
    bool caseSensitive{};
    bool wholeWord{};
    bool regularExpression{};
    int maximumMatches{5000};
};

struct ProjectSearchMatch {
    QString filePath;
    QString lineText;
    QString matchedText;
    int line{};
    int character{};
    int length{};
};

struct ProjectSearchResult {
    int requestId{};
    QVector<ProjectSearchMatch> matches;
    int fileCount{};
    int scannedFiles{};
    int skippedFiles{};
    bool truncated{};
    QString error;
};

struct ProjectReplacementFile {
    QString filePath;
    QString originalText;
    QString updatedText;
    QByteArray originalBytes;
    QByteArray updatedBytes;
    int sourceRevision{-1};
    int replacementCount{};
};

struct ProjectReplacementPlan {
    int requestId{};
    QVector<ProjectReplacementFile> files;
    QString query;
    QString replacement;
    bool caseSensitive{};
    bool wholeWord{};
    bool regularExpression{};
    int replacementCount{};
    int scannedFiles{};
    QString error;
};

Q_DECLARE_METATYPE(ProjectSearchResult)
Q_DECLARE_METATYPE(ProjectReplacementPlan)

class ProjectSearchService : public QObject {
    Q_OBJECT

public:
    explicit ProjectSearchService(QObject *parent = nullptr);
    ~ProjectSearchService() override;

    int search(const ProjectSearchRequest &request);
    int prepareReplacement(const ProjectSearchRequest &request);
    void cancel();
    void invalidateCache();

signals:
    void searchStarted(int requestId, int totalFiles);
    void searchProgress(int requestId, int scannedFiles, int totalFiles);
    void searchFinished(const ProjectSearchResult &result);
    void replacementStarted(int requestId, int totalFiles);
    void replacementProgress(int requestId, int scannedFiles, int totalFiles);
    void replacementPrepared(const ProjectReplacementPlan &plan);

private:
    QString cacheKey(const ProjectSearchRequest &request) const;

    QThreadPool m_threadPool;
    std::atomic_int m_generation{0};
    QString m_cachedKey;
    ProjectSearchResult m_cachedResult;
};
