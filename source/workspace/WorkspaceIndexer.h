#pragma once

#include <QFileSystemWatcher>
#include <QObject>
#include <QRegularExpression>
#include <QStringList>
#include <QThreadPool>
#include <QTimer>
#include <QVector>

#include <atomic>

class WorkspaceIndexer : public QObject {
    Q_OBJECT

public:
    explicit WorkspaceIndexer(QObject *parent = nullptr);
    ~WorkspaceIndexer() override;

    void setRootPath(const QString &rootPath);
    void setRootPaths(const QStringList &rootPaths);
    QString rootPath() const;
    QStringList rootPaths() const;
    void refresh();
    bool isIndexing() const;

    QStringList files() const;
    QStringList quickOpenFiles() const;
    bool isIgnoredPath(const QString &filePath) const;

signals:
    void indexUpdated();

private:
    struct IgnoreRule {
        QString basePath;
        QRegularExpression expression;
        bool negated{};
    };

    struct IndexSnapshot {
        QStringList files;
        QStringList watchDirectories;
        QStringList watchFiles;
        QStringList ignoreFiles;
        QVector<IgnoreRule> ignoreRules;
    };

    void startRefresh();
    void scheduleRefresh();
    void applySnapshot(int requestId, const IndexSnapshot &snapshot);
    void configureWatchers(const IndexSnapshot &snapshot);
    void clearWatchers();

    static IndexSnapshot buildSnapshot(
        const QStringList &rootPaths,
        int requestId,
        const std::atomic_int &generation);
    static QVector<IgnoreRule> parseIgnoreRules(
        const QString &rootPath,
        QStringList ignoreFiles);
    static bool isIgnoredByRules(
        const QString &filePath,
        const QVector<IgnoreRule> &rules);
    bool isAlwaysIgnoredPath(const QString &filePath) const;
    bool isIgnoredByWorkspaceRules(const QString &filePath) const;
    static QString globToRegularExpression(const QString &pattern);

    static constexpr int MaximumNativeWatchPaths = 4096;

    QStringList m_rootPaths;
    QStringList m_files;
    QVector<IgnoreRule> m_ignoreRules;
    QFileSystemWatcher m_fileWatcher;
    QTimer m_refreshDebounce;
    QTimer m_fallbackRefreshTimer;
    QThreadPool m_threadPool;
    std::atomic_int m_generation{0};
    bool m_indexing{};
    bool m_refreshPending{};
    bool m_updatingWatchers{};
};
