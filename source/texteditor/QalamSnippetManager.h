#pragma once

#include <QPlainTextEdit>
#include <QVector>

// Handles snippet insertion (with indentation) and Tab/Enter navigation
// through snippet placeholders. Extracted from QalamEditor.
class QalamSnippetManager {
public:
    explicit QalamSnippetManager(QPlainTextEdit *editor);

    void insertSnippet(const QString &snippet, QTextCursor &tc);

    // Try to navigate to the next snippet placeholder.
    // Returns true if navigation occurred (key event consumed).
    bool processSnippetNavigation();

    // Whether there are remaining snippet placeholders to navigate to.
    bool hasActiveSnippet() const;

private:
    QPlainTextEdit *m_editor{};
    QVector<QTextCursor> m_targets{};
};
