#pragma once

#include <QObject>
#include <QRegularExpression>
#include <QStringList>
#include <QVector>

class WorkspaceIndexer : public QObject {
    Q_OBJECT

public:
    explicit WorkspaceIndexer(QObject *parent = nullptr);

    void setRootPath(const QString &rootPath);
    QString rootPath() const;
    void refresh();

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

    void loadIgnoreRules();
    bool isAlwaysIgnoredPath(const QString &filePath) const;
    bool isIgnoredByWorkspaceRules(const QString &filePath) const;
    bool isAllowedFile(const QString &filePath) const;
    static QString globToRegularExpression(const QString &pattern);

    QString m_rootPath;
    QStringList m_files;
    QVector<IgnoreRule> m_ignoreRules;
};
