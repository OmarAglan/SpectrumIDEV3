#include "TEditor.h"

#include <QPainter>
#include <QTextBlock>
#include <QScrollBar>
#include <QMimeData>
#include <QSettings>
#include <QPainterPath>
#include <QMenu>
#include <QAction>
#include <QFile>
#include <QUrl>
#include <QHash>
#include <QToolTip>
#include <QTextDocument>
#include "Constants.h"
#include "highlighter/ThemeManager.h"
#include "highlighter/TSyntaxDefinition.h"
#include <QTextCharFormat>
#include "ui/QalamTheme.h"

#include <algorithm>


namespace {
bool isBaaCompletionCharacter(QChar ch)
{
    const ushort value = ch.unicode();
    return ch == '_' || ch == '#' ||
           (value >= 0x0600 && value <= 0x06ff) ||
           (value >= 0x0750 && value <= 0x077f) ||
           (value >= 0x08a0 && value <= 0x08ff) ||
           (value >= 0xfb50 && value <= 0xfdff) ||
           (value >= 0xfe70 && value <= 0xfeff);
}

CompletionType completionTypeForItem(const BaaCompletionItem &item)
{
    if (item.label.startsWith('#')) return CompletionType::Preprocessor;
    switch (item.kind) {
    case 3: return CompletionType::Function;
    case 5:
    case 6: return CompletionType::Variable;
    case 7:
    case 13:
    case 22:
    case 25: return CompletionType::Type;
    case 14: return CompletionType::Keyword;
    case 15: return CompletionType::Snippet;
    default: return CompletionType::Value;
    }
}
}


TEditor::TEditor(QWidget* parent)
    : QPlainTextEdit(parent),
      m_bracketHandler(this),
      m_snippetManager(this) {
    setAcceptDrops(true);
    setMouseTracking(true);
    this->setStyleSheet(QalamTheme::editorStyleSheet());
    this->setTabStopDistance(32);

    QTextDocument* editorDocument = this->document();
    QTextOption option = editorDocument->defaultTextOption();
    option.setTextDirection(Qt::RightToLeft);
    option.setAlignment(Qt::AlignRight);
    editorDocument->setDefaultTextOption(option);


    highlighter = new TSyntaxHighlighter(editorDocument);
    lineNumberArea = new LineNumberArea(this);

    // ضبط الإكمال التلقائي
    setupAutoComplete();

    connect(this, &TEditor::blockCountChanged, this, &TEditor::updateLineNumberAreaWidth);
    connect(this, &TEditor::updateRequest, this, &TEditor::updateLineNumberArea);
    connect(this, &TEditor::cursorPositionChanged, this, &TEditor::highlightCurrentLine);
    connect(this->document(), &QTextDocument::contentsChanged, this, &TEditor::updateFoldRegions);
    m_hoverTimer.setSingleShot(true);
    m_hoverTimer.setInterval(320);
    connect(&m_hoverTimer, &QTimer::timeout, this, [this]() {
        if (m_hoverRequestLine < 0 || m_hoverRequestCharacter < 0 ||
            currentFilePath().isEmpty()) return;
        emit hoverRequested(currentFilePath(),
                            m_hoverRequestLine,
                            m_hoverRequestCharacter);
    });

    updateLineNumberAreaWidth();
    highlightCurrentLine();

    // set saved setting font size to the editor
    QSettings settingsVal(Constants::OrgName, Constants::AppName);
    int savedSize = settingsVal.value(Constants::SettingsKeyFontSize).toInt();
    updateFontSize(savedSize);
    // set saved setting font type to the editor
    QString savedFont = settingsVal.value(Constants::SettingsKeyFontType).toString();
    updateFontType(savedFont);
    // set saved setting theme to the editor
    int savedThemeIdx = settingsVal.value(Constants::SettingsKeyTheme).toInt();
    if (savedThemeIdx < 0) savedThemeIdx = 0;
    auto theme = ThemeManager::getThemeByIndex(savedThemeIdx);
    updateHighlighterTheme(theme);

    // Auto-save (delegated to TAutoSave helper)
    m_autoSave = new TAutoSave(this, this);
    connect(this->document(), &QTextDocument::contentsChanged, m_autoSave, &TAutoSave::onContentChanged);
    
    installEventFilter(this);
}

void TEditor::wheelEvent(QWheelEvent *event) {
    if (event->modifiers() & Qt::ControlModifier) {
        const int delta = event->angleDelta().y();
        if (delta == 0) return;

        QFont font = this->font();
        qreal currentSize = font.pointSizeF();

        qreal step = 0.5;

        if (delta > 0) {
            currentSize += step;
        } else {
            currentSize -= step;
        }

        if (currentSize < 5.0) currentSize = 5.0;
        if (currentSize > 50) currentSize = 50;

        font.setPointSizeF(currentSize);
        this->setFont(font);

        if (lineNumberArea) {
            QFont lineFont = lineNumberArea->font();
            lineFont.setPointSizeF(currentSize);
            lineNumberArea->setFont(lineFont);
        }

        updateLineNumberAreaWidth();

        return;
    }
    QPlainTextEdit::wheelEvent(event);
}

QString TEditor::currentFilePath() const {
    return filePath;
}

void TEditor::setFilePath(const QString &path) {
    filePath = path;
    setProperty("filePath", path);
    if (m_autoSave) {
        m_autoSave->filePath = path;
    }
}

void TEditor::setDiagnostics(const QVector<Diagnostic> &diagnostics) {
    m_diagnostics = diagnostics;
    applyEditorDecorations();
    viewport()->update();
}

void TEditor::clearDiagnostics() {
    m_diagnostics.clear();
    applyEditorDecorations();
    viewport()->update();
}

void TEditor::updateFontSize(int size) {
    if (size < 10) {
        size = 18;
    }

    QFont font = this->font();
    font.setPixelSize(size);
    this->setFont(font);

    QFont fontNums = lineNumberArea->font();
    fontNums.setPixelSize(size);
    lineNumberArea->setFont(fontNums);
}

void TEditor::updateFontType(QString font) {
    QFont currentFont = this->font();
    currentFont.setFamily(font);

    this->setFont(currentFont);
}


// 1. دالة تعليق/إلغاء تعليق الأكواد
void TEditor::toggleComment()
{
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock(); // لبدء عملية تراجع (Undo) واحدة

    int startPos = cursor.selectionStart();
    int endPos = cursor.selectionEnd();

    // تحديد بداية ونهاية الأسطر المحددة
    cursor.setPosition(startPos);
    int startBlock = cursor.blockNumber();
    cursor.setPosition(endPos, QTextCursor::KeepAnchor);
    int endBlock = cursor.blockNumber();

    if (cursor.atBlockStart() && endBlock > startBlock) {
        endBlock--;
    }

    bool shouldComment = false;

    QTextBlock block = document()->findBlockByNumber(startBlock);
    if (!block.text().trimmed().startsWith("//")) {
        shouldComment = true;
    }

    for (int i = startBlock; i <= endBlock; ++i) {
        block = document()->findBlockByNumber(i);
        QTextCursor lineCursor(block);

        if (shouldComment) {
            lineCursor.movePosition(QTextCursor::StartOfBlock);
            lineCursor.insertText("// ");
        } else {
            const QString text = block.text();
            int idx = 0;
            while (idx < text.length() and text.at(idx).isSpace()) {
                ++idx;
            }

            if (text.mid(idx, 2) == "//") {
                lineCursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, idx);
                int removeCount = 2;
                if (idx + 2 < text.length() and text.at(idx + 2) == ' ') {
                    removeCount = 3;
                }
                lineCursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, removeCount);
                lineCursor.removeSelectedText();
            }
        }
    }

    cursor.endEditBlock();
}

void TEditor::duplicateLine()
{
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();

    QString lineText = cursor.block().text();

    cursor.movePosition(QTextCursor::EndOfBlock);

    cursor.insertText("\n" + lineText);

    cursor.endEditBlock();
}

void TEditor::moveLineUp()
{
    QTextCursor cursor = textCursor();
    const QTextBlock currentBlock = cursor.block();
    const QTextBlock prevBlock = currentBlock.previous();

    if (!prevBlock.isValid()) return;

    const int column = cursor.positionInBlock();
    const int targetBlockNumber = prevBlock.blockNumber();
    const QString currentText = currentBlock.text();
    const QString prevText = prevBlock.text();

    QTextCursor editCursor(prevBlock);
    editCursor.beginEditBlock();
    editCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    editCursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
    editCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    editCursor.insertText(currentText + "\n" + prevText);
    editCursor.endEditBlock();

    QTextBlock movedBlock = document()->findBlockByNumber(targetBlockNumber);
    QTextCursor newCursor(movedBlock);
    newCursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, qMin(column, static_cast<int>(currentText.length())));
    setTextCursor(newCursor);
}

void TEditor::moveLineDown()
{
    QTextCursor cursor = textCursor();
    const QTextBlock currentBlock = cursor.block();
    const QTextBlock nextBlock = currentBlock.next();

    if (!nextBlock.isValid()) return;

    const int column = cursor.positionInBlock();
    const int targetBlockNumber = nextBlock.blockNumber();
    const QString currentText = currentBlock.text();
    const QString nextText = nextBlock.text();

    QTextCursor editCursor(currentBlock);
    editCursor.beginEditBlock();
    editCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    editCursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
    editCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    editCursor.insertText(nextText + "\n" + currentText);
    editCursor.endEditBlock();

    QTextBlock movedBlock = document()->findBlockByNumber(targetBlockNumber);
    QTextCursor newCursor(movedBlock);
    newCursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, qMin(column, static_cast<int>(currentText.length())));
    setTextCursor(newCursor);
}

bool TEditor::eventFilter(QObject* obj, QEvent* event) {
    if (obj == this and event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

        if (keyEvent->key() == Qt::Key_Return
             or keyEvent->key() == Qt::Key_Enter) {
            if ((c and c->popup() and c->popup()->isVisible())
                or m_snippetManager.hasActiveSnippet()
                or (keyEvent->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier))) {
                return false;
            }

            cursorIndentation();
            return true;
        }
    }
    return QPlainTextEdit::eventFilter(obj, event);
}

void TEditor::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = createStandardContextMenu();

    menu->addSeparator();

    QAction *commentAction = new QAction("تعليق/إلغاء تعليق", this);
    commentAction->setShortcut(QKeySequence("Ctrl+/"));
    connect(commentAction, &QAction::triggered, this, &TEditor::toggleComment);
    menu->addAction(commentAction);

    QAction *duplicateAction = new QAction("تكرار السطر", this);
    duplicateAction->setShortcut(QKeySequence("Ctrl+D"));
    connect(duplicateAction, &QAction::triggered, this, &TEditor::duplicateLine);
    menu->addAction(duplicateAction);

    QAction *quickFixAction = new QAction("إصلاح سريع من باء", this);
    quickFixAction->setShortcut(QKeySequence("Ctrl+."));
    connect(quickFixAction, &QAction::triggered,
            this, &TEditor::quickFixRequested);
    menu->addAction(quickFixAction);


    menu->setStyleSheet(QString(
        "QMenu { background-color: %1; color: %2; border: 1px solid %3; }"
        "QMenu::item { padding: 5px 20px; background-color: transparent; }"
        "QMenu::item:selected { background-color: %4; color: %5; }"
        "QMenu::separator { height: 1px; background: %3; margin: 5px 0; }")
        .arg(Constants::Colors::MenuBackground)
        .arg(Constants::Colors::TextSecondary)
        .arg(Constants::Colors::Border)
        .arg(Constants::Colors::ListActiveBackground)
        .arg(Constants::Colors::TextPrimary));

    menu->exec(event->globalPos());

    delete menu;
}

int TEditor::lineNumberAreaWidth() const {
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int space = 30 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;

    return space;
}


void TEditor::updateLineNumberAreaWidth() {
    int numsWidth = lineNumberAreaWidth();

    int mapWidth = 0;

    setViewportMargins(mapWidth, 0, numsWidth, 0);
}

void TEditor::updateLineNumberArea(const QRect &rect, int dy) {
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth();
}

void TEditor::resizeEvent(QResizeEvent* event) {
    QPlainTextEdit::resizeEvent(event);

    QRect cr = contentsRect();
    int numsWidth = lineNumberAreaWidth();


    lineNumberArea->setGeometry(this->width() - numsWidth, cr.top(), numsWidth, cr.height());
}

void TEditor::lineNumberAreaPaintEvent(QPaintEvent* event) {

    QPainter painter(lineNumberArea);
    painter.fillRect(event->rect(), Qt::transparent);

        QTextBlock block = firstVisibleBlock();
        int blockNumber = block.blockNumber();
        int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
        int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);

            painter.setPen(QColor(Constants::Colors::TextMuted));

            painter.drawText(12, top, lineNumberArea->width(), fontMetrics().height(),
                                     Qt::AlignRight | Qt::AlignVCenter, number);

            for (const auto& region : foldRegions) {
                if (region.startBlockNumber == blockNumber) {
                    bool folded = region.folded;

                    QPolygon arrow;
                    int midY = top + fontMetrics().height() / 2;
                    if (folded) {
                        arrow << QPoint(lineNumberArea->width() - 10, midY - 4)
                        << QPoint(lineNumberArea->width() - 2, midY)
                        << QPoint(lineNumberArea->width() - 10, midY + 4);
                    } else {
                        arrow << QPoint(lineNumberArea->width() - 10, midY - 4)
                        << QPoint(lineNumberArea->width() - 2, midY - 4)
                        << QPoint(lineNumberArea->width() - 6, midY + 4);
                    }

                    painter.setBrush(QColor(Constants::Colors::Accent));
                    painter.setPen(Qt::NoPen);
                    painter.drawPolygon(arrow);
                }
            }
        }

        block = block.next();
        top = bottom;
        bottom = top + static_cast<int>(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void TEditor::highlightCurrentLine() {
    applyEditorDecorations();
}

void TEditor::applyEditorDecorations() {
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        QColor lineColor = QColor(Constants::Colors::CurrentLineHighlight);
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    for (const Diagnostic &diagnostic : m_diagnostics) {
        const QTextBlock block = document()->findBlockByNumber(qMax(1, diagnostic.line) - 1);
        if (!block.isValid()) continue;

        QTextCursor cursor(block);
        const int columnOffset = qMax(0, diagnostic.column - 1);
        cursor.setPosition(qMin(block.position() + columnOffset, block.position() + block.length() - 1));

        // Underline the closest token. If there is no token at that column,
        // underline the rest of the line so the diagnostic remains visible.
        QTextCursor wordCursor = cursor;
        wordCursor.select(QTextCursor::WordUnderCursor);
        if (!wordCursor.hasSelection() || wordCursor.selectedText().trimmed().isEmpty()) {
            wordCursor = cursor;
            wordCursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            if (!wordCursor.hasSelection()) {
                wordCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
            }
        }

        QTextEdit::ExtraSelection diagnosticSelection;
        diagnosticSelection.cursor = wordCursor;
        diagnosticSelection.format.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
        diagnosticSelection.format.setUnderlineColor(
            diagnostic.severity == "warning"
                ? QColor(Constants::Colors::WarningForeground)
                : QColor(Constants::Colors::ErrorForeground));
        diagnosticSelection.format.setToolTip(diagnostic.message);
        extraSelections.append(diagnosticSelection);
    }

    setExtraSelections(extraSelections);
}

TEditor::Diagnostic TEditor::diagnosticAtPosition(const QPoint &position) const {
    Diagnostic result;
    const QTextCursor cursor = cursorForPosition(position);
    const int line = cursor.blockNumber() + 1;
    const int column = cursor.positionInBlock() + 1;

    for (const Diagnostic &diagnostic : m_diagnostics) {
        if (diagnostic.line != line) continue;
        // Hover anywhere on the diagnostic line, but prefer the right-side text
        // range around the reported column.
        if (column >= qMax(1, diagnostic.column - 2)) {
            return diagnostic;
        }
    }
    return result;
}

bool TEditor::hasDiagnosticAtPosition(const QPoint &position, Diagnostic *diagnostic) const {
    const Diagnostic found = diagnosticAtPosition(position);
    if (found.message.isEmpty()) return false;
    if (diagnostic) *diagnostic = found;
    return true;
}

void TEditor::updateFoldRegions() {

    QHash<int, bool> previousFoldStates;
    for (const FoldRegion& region : foldRegions) {
        previousFoldStates[region.startBlockNumber] = region.folded;
    }

    foldRegions.clear();

    for (QTextBlock visibleBlock = document()->firstBlock(); visibleBlock.isValid(); visibleBlock = visibleBlock.next()) {
        visibleBlock.setVisible(true);
    }

    QTextBlock block = document()->firstBlock();
    while (block.isValid()) {
        QString text = block.text();

        QString trimmed = text.trimmed();
        if (trimmed.startsWith("دالة ") || trimmed.startsWith("صنف ")) {
            int start = block.blockNumber();

            int startIndent = 0;
            for (QChar c : text) {
                if (c == '\t') startIndent += 4;
                else if (c == ' ') startIndent += 1;
                else break;
            }

            QTextBlock next = block.next();
            int end = start;

            while (next.isValid()) {
                QString nextText = next.text();
                QString nextTrim = nextText.trimmed();

                if (nextTrim.isEmpty()) {
                    next = next.next();
                    continue;
                }

                int nextIndent = 0;
                for (QChar c : nextText) {
                    if (c == '\t') nextIndent += 4;
                    else if (c == ' ') nextIndent += 1;
                    else break;
                }

                if (nextTrim.startsWith("دالة ") || nextTrim.startsWith("صنف ")) {
                    if (nextIndent <= startIndent)
                        break;
                }

                if (nextIndent <= startIndent)
                    break;

                end = next.blockNumber();
                next = next.next();
            }

            if (end > start) {
                FoldRegion region{};
                region.startBlockNumber = start;
                region.endBlockNumber = end;
                region.folded = false;
                if (previousFoldStates.contains(region.startBlockNumber))
                    region.folded = previousFoldStates[region.startBlockNumber];
                foldRegions.append(region);
            }
        }
        block = block.next();
    }

    if (lineNumberArea)
        lineNumberArea->update();

    for (const FoldRegion& region : foldRegions) {
        QTextBlock block = document()->findBlockByNumber(region.startBlockNumber + 1);
        while (block.isValid() && block.blockNumber() <= region.endBlockNumber) {
            block.setVisible(!region.folded);
            block = block.next();
        }
    }
    viewport()->update();
}

void TEditor::toggleFold(int blockNumber) {
    for (FoldRegion &region : foldRegions) {
        if (region.startBlockNumber == blockNumber) {
            region.folded = !region.folded;

            QTextBlock block = document()->findBlockByNumber(region.startBlockNumber + 1);
            while (block.isValid() && block.blockNumber() <= region.endBlockNumber) {
                block.setVisible(!region.folded);
                block = block.next();
            }

            if (!region.folded) {
                for (FoldRegion &subRegion : foldRegions) {
                    if (subRegion.startBlockNumber > region.startBlockNumber &&
                        subRegion.endBlockNumber <= region.endBlockNumber) {
                        QTextBlock subBlock = document()->findBlockByNumber(subRegion.startBlockNumber + 1);
                        bool allVisible = true;
                        while (subBlock.isValid() && subBlock.blockNumber() <= subRegion.endBlockNumber) {
                            if (!subBlock.isVisible()) {
                                allVisible = false;
                                break;
                            }
                            subBlock = subBlock.next();
                        }
                        subRegion.folded = !allVisible;
                    }
                }
            }

            viewport()->update();
            break;
        }
    }
}


/* ---------------------------------- Drag and Drop ---------------------------------- */

void TEditor::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        for (const QUrl& url : event->mimeData()->urls()) {
            if (url.fileName().endsWith(".baa", Qt::CaseInsensitive) or
                url.fileName().endsWith(".baahd", Qt::CaseInsensitive) or
                url.fileName().endsWith(".txt", Qt::CaseInsensitive)) {
                event->acceptProposedAction();
                return;
            }
        }
    }

    if (event->mimeData()->hasText()) {
        event->acceptProposedAction();
        return;
    }
    event->ignore();
}

void TEditor::dragMoveEvent(QDragMoveEvent* event) {
    event->acceptProposedAction();
}

void TEditor::dropEvent(QDropEvent* event) {
    if (event->mimeData()->hasUrls()) {
        for (const QUrl& url : event->mimeData()->urls()) {
            if (url.fileName().endsWith(".baa", Qt::CaseInsensitive) or
                url.fileName().endsWith(".baahd", Qt::CaseInsensitive) or
                url.fileName().endsWith(".txt", Qt::CaseInsensitive)) {

                QString filePath = url.toLocalFile();
                emit openRequest(filePath);

                event->acceptProposedAction();
                return;
            }
        }
    }

    if (event->mimeData()->hasText()) {
        QTextCursor dropCursor = cursorForPosition(event->position().toPoint());
        int dropPosition = dropCursor.position();

        QTextCursor originalCursor = textCursor();
        const bool movingSelection = originalCursor.hasSelection();
        const int selectionStart = originalCursor.selectionStart();
        const int selectionEnd = originalCursor.selectionEnd();

        if (movingSelection
            and dropPosition >= selectionStart
            and dropPosition <= selectionEnd) {
            event->ignore();
            return;
        }

        QString droppedText = event->mimeData()->text();

        if (movingSelection) {
            originalCursor.removeSelectedText();
            if (selectionStart < dropPosition) {
                dropPosition -= droppedText.length();
            }
        }

        dropCursor.setPosition(qMax(0, dropPosition));
        dropCursor.insertText(droppedText);

        event->acceptProposedAction();
        return;
    }

    event->ignore();
}

void TEditor::dragLeaveEvent(QDragLeaveEvent* event) {
    event->accept();
}


void TEditor::mouseMoveEvent(QMouseEvent *event) {
    Diagnostic diagnostic;
    if (hasDiagnosticAtPosition(event->position().toPoint(), &diagnostic)) {
        m_hoverTimer.stop();
        m_hoverRequestLine = -1;
        m_hoverRequestCharacter = -1;
        const QString prefix = diagnostic.severity == "warning" ? "تحذير" : "خطأ";
        QToolTip::showText(event->globalPosition().toPoint(),
                           QString("%1: %2").arg(prefix, diagnostic.message),
                           viewport());
    } else {
        scheduleLanguageHover(event->position().toPoint(),
                              event->globalPosition().toPoint());
    }
    QPlainTextEdit::mouseMoveEvent(event);
}

void TEditor::leaveEvent(QEvent *event) {
    clearSemanticPresentation();
    QPlainTextEdit::leaveEvent(event);
}

void TEditor::scheduleLanguageHover(const QPoint &viewportPosition,
                                    const QPoint &globalPosition)
{
    if (currentFilePath().isEmpty()) {
        clearSemanticPresentation();
        return;
    }
    const QTextCursor cursor = cursorForPosition(viewportPosition);
    const QString text = cursor.block().text();
    int character = qBound(0, cursor.positionInBlock(), text.size());
    if (character < text.size() &&
        isBaaCompletionCharacter(text.at(character))) {
        // The insertion position already points inside the identifier.
    } else if (character > 0 &&
               isBaaCompletionCharacter(text.at(character - 1))) {
        --character;
    } else {
        clearSemanticPresentation();
        return;
    }

    const int line = cursor.blockNumber();
    const bool changed = line != m_hoverRequestLine ||
                         character != m_hoverRequestCharacter;
    m_hoverRequestLine = line;
    m_hoverRequestCharacter = character;
    m_hoverGlobalPosition = globalPosition;
    if (changed) QToolTip::hideText();
    m_hoverTimer.start();
}

void TEditor::showLanguageHover(const BaaHover &hover,
                                int requestLine,
                                int requestCharacter)
{
    if (requestLine != m_hoverRequestLine ||
        requestCharacter != m_hoverRequestCharacter) return;
    if (!hover.isValid()) {
        QToolTip::hideText();
        return;
    }

    QTextDocument document;
    document.setDefaultStyleSheet(
        QStringLiteral("body{direction:rtl;text-align:right;"
                       "font-family:'Noto Sans Arabic','Segoe UI';}"
                       "pre{direction:rtl;text-align:right;"
                       "background:#20242b;padding:8px;border-radius:5px;}"));
    if (hover.contentKind == QStringLiteral("markdown"))
        document.setMarkdown(hover.contents);
    else
        document.setPlainText(hover.contents);
    document.setTextWidth(420);
    QToolTip::showText(m_hoverGlobalPosition,
                       document.toHtml(),
                       viewport());
}

void TEditor::showSignatureHelp(const BaaSignatureHelp &signatureHelp,
                                int requestLine,
                                int requestCharacter)
{
    const QTextCursor cursor = textCursor();
    if (cursor.blockNumber() != requestLine ||
        cursor.positionInBlock() != requestCharacter) return;
    if (!signatureHelp.isValid()) {
        QToolTip::hideText();
        return;
    }

    QString details;
    if (!signatureHelp.parameters.isEmpty()) {
        const int active = qBound(0, signatureHelp.activeParameter,
                                  signatureHelp.parameters.size() - 1);
        details = QStringLiteral(
            "<div style='margin-top:6px;color:#9fc7ff'>"
            "المعامل %1 من %2: <b>%3</b></div>")
            .arg(active + 1)
            .arg(signatureHelp.parameters.size())
            .arg(signatureHelp.parameters.at(active).toHtmlEscaped());
    }
    const QString html = QStringLiteral(
        "<div dir='rtl' style='text-align:right;white-space:pre-wrap'>"
        "<b>%1</b>%2</div>")
        .arg(signatureHelp.label.toHtmlEscaped(), details);
    const QPoint position = viewport()->mapToGlobal(
        cursorRect().bottomLeft() + QPoint(0, 5));
    QToolTip::showText(position, html, viewport());
}

void TEditor::requestSignatureHelp()
{
    if (currentFilePath().isEmpty()) return;
    const QTextCursor cursor = textCursor();
    emit signatureHelpRequested(currentFilePath(),
                                cursor.blockNumber(),
                                cursor.positionInBlock());
}

void TEditor::clearSemanticPresentation()
{
    m_hoverTimer.stop();
    m_hoverRequestLine = -1;
    m_hoverRequestCharacter = -1;
    QToolTip::hideText();
}


/* ---------------------------------- Indentation ---------------------------------- */

void TEditor::cursorIndentation() {
    QTextCursor cursor = textCursor();
    QString lineText = cursor.block().text();
    int cursorPosInLine = cursor.positionInBlock();
    QString currentIndentation = getCurrentLineIndentation(cursor);

    if (cursorPosInLine > 0) {
        int checkPos = cursorPosInLine - 1;
        while (checkPos >= 0 and lineText.at(checkPos).isSpace()) {
            checkPos--;
        }

        if (checkPos >= 0 and lineText.at(checkPos) == ':') {
            currentIndentation += "\t";
        } else {
            // Also indent after function, class definitions, or control blocks starting with {
            QString trimmed = lineText.trimmed();
            if (trimmed.startsWith("إذا") || trimmed.startsWith("طالما") || 
                trimmed.startsWith("لكل") || trimmed.endsWith("{")) {
                currentIndentation += "\t";
            }
        }
    }

    cursor.beginEditBlock();
    cursor.insertText("\n" + currentIndentation);
    cursor.endEditBlock();
    setTextCursor(cursor);
}

QString TEditor::getCurrentLineIndentation(const QTextCursor &cursor) const {
    QTextBlock block = cursor.block();
    if (!block.isValid()) {
        return QString();
    }

    QString lineText = block.text();
    QString indentation;
    for (const QChar &ch : lineText) {
        if (ch == ' ' or ch == '\t') {
            indentation += ch;
        } else {
            break;
        }
    }
    return indentation;
}


/* ---------------------------------- Auto-Save (delegated) ---------------------------------- */

void TEditor::startAutoSave() {
    m_autoSave->filePath = currentFilePath();
    m_autoSave->start();
}

void TEditor::stopAutoSave() {
    m_autoSave->stop();
}

void TEditor::removeBackupFile() {
    m_autoSave->filePath = currentFilePath();
    m_autoSave->removeBackupFile();
}


void TEditor::updateHighlighterTheme(std::shared_ptr<SyntaxTheme> theme) {
    this->highlighter->setTheme(theme);
}



// --- autocomplete system ---

void TEditor::setupAutoComplete() {
    model = new CompletionModel(this);
    QCompleter *completer = new QCompleter(this);
    setCompleter(completer);
}

void TEditor::setCompleter(QCompleter *completer) {
    if (c) disconnect(c, nullptr, this, nullptr);
    c = completer;
    if (!c) return;

    c->setWidget(this);
    c->setCompletionMode(QCompleter::PopupCompletion);
    c->setCaseSensitivity(Qt::CaseInsensitive);
    c->setModel(model);

    // Custom Rich Popup ---
    TCompletionPopup *popup = new TCompletionPopup;
    c->setPopup(popup); // QCompleter takes ownership

    popup->setItemDelegate(new TModernCompletionDelegate(popup));

    // set dimensions
    popup->setMinimumWidth(320);
    popup->setMinimumHeight(150);


    // To this lambda that captures the type:
    connect(c, QOverload<const QString &>::of(&QCompleter::activated),
            this, [this](const QString &) {
                QModelIndex index = c->popup()->currentIndex();
                if (not index.isValid()) return;
                const CompletionItem *item = model->itemAt(index.row());
                if (item) insertCompletion(*item);
            });
}

void TEditor::focusOutEvent(QFocusEvent *e) {
    if (c && c->popup()->isVisible()) {
        c->popup()->hide();
    }
    QPlainTextEdit::focusOutEvent(e);
}

void TEditor::keyPressEvent(QKeyEvent *e) {
    const bool signatureShortcut =
        (e->modifiers() & Qt::ControlModifier) &&
        (e->modifiers() & Qt::ShiftModifier) &&
        e->key() == Qt::Key_Space;
    if (signatureShortcut) {
        requestSignatureHelp();
        e->accept();
        return;
    }

    // Bracket and quote auto-pairing (delegated to TBracketHandler)
    if (m_bracketHandler.handleAutoPairing(e)) {
        if (e->text().contains('(')) requestSignatureHelp();
        if (e->text().contains(')')) QToolTip::hideText();
        e->accept();
        return;
    }

    // RTL-Aware Word Navigation (Alt + Arrow Keys)
    if (e->modifiers() & Qt::AltModifier) {
        QTextCursor cursor = textCursor();
        if (e->key() == Qt::Key_Left) {
            cursor.movePosition(QTextCursor::WordLeft);
            setTextCursor(cursor);
            e->accept();
            return;
        }
        if (e->key() == Qt::Key_Right) {
            cursor.movePosition(QTextCursor::WordRight);
            setTextCursor(cursor);
            e->accept();
            return;
        }
    }

    // Handle Navigation for Live Update (Arrow Keys) ---
    if (e->key() == Qt::Key_Left || e->key() == Qt::Key_Right) {
        // Let the editor move the cursor first
        QPlainTextEdit::keyPressEvent(e);
        // Then immediately trigger completion to update the list based on the new cursor position
        performCompletion();
        return;
    }

    if (c && c->popup()->isVisible()) {
        switch (e->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
        case Qt::Key_Escape:
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
            e->ignore();
            return;
        default: break;
        }
    }

    // Snippet navigation (delegated to TSnippetManager)
    if ((e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter)) {
        if (m_snippetManager.hasActiveSnippet()) {
            if (m_snippetManager.processSnippetNavigation()) {
                e->accept();
                return;
            }
        }
    }

    if (e->key() == Qt::Key_Tab && m_snippetManager.hasActiveSnippet()) {
        if (m_snippetManager.processSnippetNavigation()) {
            e->accept();
            return;
        }
    }

    const bool isShortcut =
        (e->modifiers() & Qt::ControlModifier) &&
        !(e->modifiers() & Qt::ShiftModifier) &&
        e->key() == Qt::Key_Space;

    QPlainTextEdit::keyPressEvent(e);

    if (isShortcut) {
        performCompletion(true);
        return;
    }
    if (e->key() == Qt::Key_Escape || e->text().contains(')')) {
        QToolTip::hideText();
    }
    if (e->text().isEmpty()) return;

    bool hasArabicTrigger = false;
    bool hasSignatureTrigger = false;
    for (const QChar character : e->text()) {
        if (isBaaCompletionCharacter(character)) {
            hasArabicTrigger = true;
        }
        if (character == '(' || character == ',' || character == QChar(0x060c))
            hasSignatureTrigger = true;
    }
    if (hasSignatureTrigger) requestSignatureHelp();
    if (hasArabicTrigger) performCompletion();
    else if (c and c->popup()) c->popup()->hide();
}

void TEditor::performCompletion(bool explicitRequest) {
    if (not c or not model) return;
    const QString prefix = textUnderCursor();
    if (not explicitRequest and prefix.isEmpty()) {
        c->popup()->hide();
        return;
    }
    if (currentFilePath().isEmpty()) {
        c->popup()->hide();
        return;
    }
    c->popup()->hide();
    const QTextCursor cursor = textCursor();
    emit completionRequested(currentFilePath(), cursor.blockNumber(),
                             cursor.positionInBlock());
}

void TEditor::showLanguageCompletions(const QVector<BaaCompletionItem> &items,
                                      int line,
                                      int character)
{
    const QTextCursor cursor = textCursor();
    if (cursor.blockNumber() != line or cursor.positionInBlock() != character) return;

    std::vector<CompletionItem> completions;
    completions.reserve(static_cast<size_t>(items.size()));
    for (const BaaCompletionItem &source : items) {
        CompletionItem item;
        item.label = source.label;
        item.completion = source.newText;
        item.description = source.detail;
        item.type = completionTypeForItem(source);
        item.snippet = source.insertTextFormat == 2;
        item.startLine = source.startLine;
        item.startCharacter = source.startCharacter;
        item.endLine = source.endLine;
        item.endCharacter = source.endCharacter;
        completions.push_back(std::move(item));
    }
    model->updateData(completions);
    if (completions.empty()) {
        c->popup()->hide();
        return;
    }
    c->setCompletionPrefix(QString());
    showCompletionPopup();
}

bool TEditor::hasVisibleCompletion() const
{
    return c and c->popup() and c->popup()->isVisible();
}

void TEditor::showCompletionPopup()
{
    QRect cr = cursorRect();

    QPoint widgetPos = this->viewport()->mapTo(this, cr.topRight());
    cr.moveTo(widgetPos);

    // Calculate popup width with constants
    int popupWidth = std::clamp(
        Constants::Layout::PopupBasePadding + c->popup()->verticalScrollBar()->width(),
        Constants::Layout::PopupMinWidth,
        Constants::Layout::PopupMaxWidth);

    // RTL positioning: place popup to the left of cursor
    // Calculate position based on actual viewport/widget geometry
    int cursorX = cr.x();
    
    // In RTL mode, position popup so it appears to the left of the cursor
    // Ensure popup stays within visible area
    int popupX = cursorX - popupWidth;
    if (popupX < 0) {
        popupX = 0;  // Don't go past left edge
    }
    
    cr.moveLeft(popupX);
    cr.setWidth(popupWidth);

    c->complete(cr);
}

QString TEditor::textUnderCursor() const {
    const QTextCursor cursor = textCursor();
    const QString blockText = cursor.block().text();
    int end = qBound(0, cursor.positionInBlock(), static_cast<int>(blockText.length()));
    int start = end;

    while (start > 0 and isBaaCompletionCharacter(blockText.at(start - 1))) {
        --start;
    }

    return blockText.mid(start, end - start);
}

int TEditor::documentPosition(int zeroBasedLine, int utf16Character) const
{
    const QTextBlock block = document()->findBlockByNumber(zeroBasedLine);
    if (not block.isValid()) return -1;
    return block.position() + qBound(0, utf16Character, block.text().length());
}

void TEditor::insertCompletion(const CompletionItem &item) {
    if (not c or c->widget() != this) return;
    const int start = documentPosition(item.startLine, item.startCharacter);
    const int end = documentPosition(item.endLine, item.endCharacter);
    if (start < 0 or end < start) return;

    QTextCursor tc = textCursor();
    tc.setPosition(start);
    tc.setPosition(end, QTextCursor::KeepAnchor);
    if (item.snippet) m_snippetManager.insertSnippet(item.completion, tc);
    else {
        tc.insertText(item.completion);
        setTextCursor(tc);
    }
}
