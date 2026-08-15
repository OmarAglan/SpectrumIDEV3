#include "QalamAutoSave.h"

#include <QPlainTextEdit>
#include <QFile>
#include <QTextStream>
#include <QStringConverter>
#include "Constants.h"

QalamAutoSave::QalamAutoSave(QPlainTextEdit *editor, QObject *parent)
    : QObject(parent), m_editor(editor) {
    m_timer = new QTimer(this);
    m_timer->setInterval(Constants::Timing::AutoSaveInterval);
    connect(m_timer, &QTimer::timeout, this, &QalamAutoSave::performAutoSave);
}

void QalamAutoSave::start() {
    if (!m_timer->isActive()) {
        m_timer->start();
    }
}

void QalamAutoSave::stop() {
    m_timer->stop();
}

void QalamAutoSave::onContentChanged() {
    start();
}

void QalamAutoSave::performAutoSave() {
    if (filePath.isEmpty() or !m_editor->document()->isModified()) return;

    QString backupPath = filePath + Constants::BackupExtension;

    QFile file(backupPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out.setEncoding(QStringConverter::Utf8);
        out << m_editor->toPlainText();
        file.close();
    }
}

void QalamAutoSave::removeBackupFile() {
    if (filePath.isEmpty()) return;

    QString backupPath = filePath + Constants::BackupExtension;
    if (QFile::exists(backupPath)) {
        QFile::remove(backupPath);
    }
    stop();
}
