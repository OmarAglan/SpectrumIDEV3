#include "DiagnosticsModel.h"

#include <QDir>
#include <QSet>
#include <utility>

DiagnosticsModel::DiagnosticsModel(QObject *parent)
    : QObject(parent)
{
}

void DiagnosticsModel::clear()
{
    if (m_diagnostics.isEmpty()) return;
    m_diagnostics.clear();
    emit diagnosticsChanged();
}

void DiagnosticsModel::setDiagnostics(const QVector<Diagnostic> &diagnostics)
{
    m_diagnostics = diagnostics;
    emit diagnosticsChanged();
}

void DiagnosticsModel::addDiagnostics(const QVector<Diagnostic> &diagnostics)
{
    if (diagnostics.isEmpty()) return;

    QSet<QString> seen;
    for (const Diagnostic &diagnostic : std::as_const(m_diagnostics)) {
        seen.insert(diagnostic.key());
    }

    bool changed = false;
    for (const Diagnostic &diagnostic : diagnostics) {
        const QString key = diagnostic.key();
        if (seen.contains(key)) continue;
        seen.insert(key);
        m_diagnostics.push_back(diagnostic);
        changed = true;
    }

    if (changed) emit diagnosticsChanged();
}

void DiagnosticsModel::replaceDiagnosticsFromSource(
    const QString &source,
    const QVector<Diagnostic> &diagnostics)
{
    if (source.isEmpty()) return;
    QVector<Diagnostic> updated;
    updated.reserve(m_diagnostics.size() + diagnostics.size());

    for (const Diagnostic &diagnostic : std::as_const(m_diagnostics)) {
        if (diagnostic.source != source) updated.push_back(diagnostic);
    }

    QSet<QString> seen;
    for (Diagnostic diagnostic : diagnostics) {
        diagnostic.source = source;
        const QString key = diagnostic.key();
        if (seen.contains(key)) continue;
        seen.insert(key);
        updated.push_back(diagnostic);
    }

    m_diagnostics = updated;
    emit diagnosticsChanged();
}

QVector<Diagnostic> DiagnosticsModel::diagnostics() const
{
    return m_diagnostics;
}

QVector<Diagnostic> DiagnosticsModel::diagnosticsForFile(const QString &filePath) const
{
    QVector<Diagnostic> result;
    const QString target = QDir::cleanPath(filePath);
    for (const Diagnostic &diagnostic : m_diagnostics) {
        if (QDir::cleanPath(diagnostic.file) == target) {
            result.push_back(diagnostic);
        }
    }
    return result;
}

int DiagnosticsModel::errorCount() const
{
    int total = 0;
    for (const Diagnostic &diagnostic : m_diagnostics) {
        if (diagnostic.isError()) ++total;
    }
    return total;
}

int DiagnosticsModel::warningCount() const
{
    int total = 0;
    for (const Diagnostic &diagnostic : m_diagnostics) {
        if (diagnostic.isWarning()) ++total;
    }
    return total;
}

int DiagnosticsModel::count() const
{
    return m_diagnostics.size();
}
