#pragma once

#include <QScrollBar>
#include <QPlainTextEdit>
#include <QCompleter>
#include <QPoint>
#include <QTimer>
#include <QVector>

#include <memory>

#include "QalamSettings.h"
#include "QalamSyntaxHighlighter.h"
#include "AutoComplete.h"
#include "AutoCompleteUI.h"
#include "QalamBracketHandler.h"
#include "QalamAutoSave.h"
#include "QalamSnippetManager.h"
#include "Constants.h"
#include "BaaCompletionItem.h"
#include "BaaHover.h"
#include "BaaInlayHint.h"
#include "BaaFoldingRange.h"
#include "BaaSelectionRange.h"
#include "BaaSignatureHelp.h"
#include "BaaSemanticToken.h"


class LineNumberArea;


class QalamEditor : public QPlainTextEdit {
    Q_OBJECT

public:
    struct Diagnostic {
        QString file;
        int line = 1;
        int column = 1;
        QString severity;
        QString message;
    };

    QalamEditor(QWidget* parent = nullptr);

    void setDiagnostics(const QVector<Diagnostic> &diagnostics);
    void clearDiagnostics();
    void setSemanticTokens(const QVector<BaaSemanticToken> &tokens);
    void clearSemanticTokens();
    void setFoldingRanges(const QVector<BaaFoldingRange> &ranges);
    void clearFoldingRanges();
    void useLocalFoldingRanges();
    int foldingRangeCount() const { return foldRegions.size(); }
    void setInlayHints(const QVector<BaaInlayHint> &hints);
    void clearInlayHints();
    int inlayHintCount() const { return m_inlayHints.size(); }
    void applySemanticSelectionRanges(
        const QVector<BaaSelectionRange> &ranges,
        int requestLine,
        int requestCharacter);
    void expandSemanticSelection();
    void shrinkSemanticSelection();

    void lineNumberAreaPaintEvent(QPaintEvent* event);
    int lineNumberAreaWidth() const;
    QString filePath{};
    QString currentFilePath() const;
    void setFilePath(const QString &path);

    QString getCurrentLineIndentation(const QTextCursor &cursor) const;
    void cursorIndentation();

    void setCompleter(QCompleter *completer);
    void showLanguageCompletions(const QVector<BaaCompletionItem> &items,
                                 int line,
                                 int character);
    bool hasVisibleCompletion() const;
    void showLanguageHover(const BaaHover &hover,
                           int requestLine,
                           int requestCharacter);
    void showSignatureHelp(const BaaSignatureHelp &signatureHelp,
                           int requestLine,
                           int requestCharacter);

    void startAutoSave();
    void stopAutoSave();
    void removeBackupFile();

public slots:
    void updateFontSize(int);
    void updateFontType(QString font);
    void toggleComment();
    void duplicateLine();
    void moveLineUp();
    void moveLineDown();
    void updateHighlighterTheme(std::shared_ptr<SyntaxTheme>);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

    void keyPressEvent(QKeyEvent *e) override;
    // We override focusOutEvent to close the popup if the user clicks away
    void focusOutEvent(QFocusEvent *e) override;

private:
    QalamSyntaxHighlighter* highlighter{};

    LineNumberArea* lineNumberArea{};

    struct FoldRegion {
        int startBlockNumber;
        int endBlockNumber;
        bool folded = false;
    };
    QVector<FoldRegion> foldRegions;

    void updateFoldRegions();
    void toggleFold(int blockNum);
    void replaceFoldRegions(const QVector<FoldRegion> &regions);
    void applyFoldVisibility();
    void applyEditorDecorations();
    Diagnostic diagnosticAtPosition(const QPoint &position) const;
    bool hasDiagnosticAtPosition(const QPoint &position, Diagnostic *diagnostic) const;

    // Extracted helpers
    QalamBracketHandler m_bracketHandler;
    QalamAutoSave *m_autoSave{};
    QalamSnippetManager m_snippetManager;

    friend class LineNumberArea;

    QCompleter* c{};
    CompletionModel *model{};
    QVector<Diagnostic> m_diagnostics;
    QVector<BaaInlayHint> m_inlayHints;
    QString textUnderCursor() const;
    void performCompletion(bool explicitRequest = false);
    void showCompletionPopup();
    void setupAutoComplete();
    int documentPosition(int zeroBasedLine, int utf16Character) const;
    void insertCompletion(const CompletionItem &item);
    void scheduleLanguageHover(const QPoint &viewportPosition,
                               const QPoint &globalPosition);
    void requestSignatureHelp();
    void clearSemanticPresentation();
    QTimer m_hoverTimer;
    int m_hoverRequestLine{-1};
    int m_hoverRequestCharacter{-1};
    QPoint m_hoverGlobalPosition;
    QVector<BaaSelectionRange> m_semanticSelectionRanges;
    QVector<QPair<int, int>> m_semanticSelectionHistory;
    int m_selectionRequestLine{-1};
    int m_selectionRequestCharacter{-1};

private slots:
    void updateLineNumberAreaWidth();
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);
signals:
    void openRequest(QString filePath);
    void quickFixRequested();
    void formatRequested();
    void completionRequested(QString filePath, int line, int character);
    void hoverRequested(QString filePath, int line, int character);
    void signatureHelpRequested(QString filePath, int line, int character);
    void selectionRangeRequested(QString filePath, int line, int character);
};


class LineNumberArea : public QWidget {
public:
    LineNumberArea(QalamEditor* editor) : QWidget(editor), qalamEditor(editor) {
        this->setStyleSheet(QString(
            "QWidget {"
            "   border-left: 1px solid %1;"
            "   border-top-left-radius: 9px;"
            "   border-bottom-left-radius: 9px;"
            "}").arg(Constants::Colors::LineNumberBorder));
    }

    QSize sizeHint() const override {
        return QSize(qalamEditor->lineNumberAreaWidth(), 0);
    }

    void mousePressEvent(QMouseEvent* event) override {
        int y = event->position().y();
        QTextBlock block = qalamEditor->firstVisibleBlock();
        int top = qRound(qalamEditor->blockBoundingGeometry(block).translated(qalamEditor->contentOffset()).top());
        int height = qRound(qalamEditor->blockBoundingRect(block).height());

        while (block.isValid() && top <= y) {
            if (y >= top && y < top + height) {
                int blockNum = block.blockNumber();
                qalamEditor->toggleFold(blockNum);
                return;
            }
            block = block.next();
            top += height;
            height = qRound(qalamEditor->blockBoundingRect(block).height());
        }
    }


protected:
    void paintEvent(QPaintEvent* event) override {
        qalamEditor->lineNumberAreaPaintEvent(event);
    }


private:
    QalamEditor* qalamEditor{};
};
