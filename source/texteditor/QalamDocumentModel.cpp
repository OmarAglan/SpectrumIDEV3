#include "QalamDocumentModel.h"

#include "Constants.h"
#include "QalamSyntaxHighlighter.h"

#include <QTextDocument>
#include <QTextOption>
#include <QPlainTextDocumentLayout>

QalamDocumentModel::QalamDocumentModel(QObject *parent)
    : QObject(parent),
      m_document(new QTextDocument(this)),
      m_highlighter(new QalamSyntaxHighlighter(m_document))
{
    m_document->setDocumentLayout(new QPlainTextDocumentLayout(m_document));
    QTextOption option = m_document->defaultTextOption();
    option.setTextDirection(Qt::RightToLeft);
    option.setAlignment(Qt::AlignRight);
    m_document->setDefaultTextOption(option);
}

void QalamDocumentModel::setFilePath(const QString &path)
{
    if (m_filePath == path) return;
    m_filePath = path;

    if (path.endsWith(QStringLiteral(".نظم"), Qt::CaseInsensitive)) {
        m_highlighter->setLanguageMode(QalamLanguageMode::Nazm);
    } else if (Constants::isBaaDocumentPath(path) or path.isEmpty()) {
        m_highlighter->setLanguageMode(QalamLanguageMode::Baa);
    } else {
        m_highlighter->setLanguageMode(QalamLanguageMode::PlainText);
    }
    emit filePathChanged(path);
}
