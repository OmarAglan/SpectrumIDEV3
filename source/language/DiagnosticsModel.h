#pragma once

#include "Diagnostic.h"

#include <QObject>
#include <QVector>

class DiagnosticsModel : public QObject
{
    Q_OBJECT

public:
    explicit DiagnosticsModel(QObject *parent = nullptr);

    void clear();
    void setDiagnostics(const QVector<Diagnostic> &diagnostics);
    void addDiagnostics(const QVector<Diagnostic> &diagnostics);
    void replaceDiagnosticsFromSource(const QString &source,
                                      const QVector<Diagnostic> &diagnostics);

    QVector<Diagnostic> diagnostics() const;
    QVector<Diagnostic> diagnosticsForFile(const QString &filePath) const;
    int errorCount() const;
    int warningCount() const;
    int count() const;

signals:
    void diagnosticsChanged();

private:
    QVector<Diagnostic> m_diagnostics;
};
