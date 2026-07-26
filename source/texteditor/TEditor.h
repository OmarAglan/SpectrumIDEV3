#pragma once

#include <QScrollBar>
#include <QPlainTextEdit>
#include <QCompleter>
#include <QPoint>
#include <QTimer>
#include <QVector>

#include <memory>

#include "TSettings.h"
#include "TSyntaxHighlighter.h"
#include "AutoComplete.h"
#include "AutoCompleteUI.h"
#include "TBracketHandler.h"
#include "TAutoSave.h"
#include "TSnippetManager.h"
#include "Constants.h"
#include "BaaCompletionItem.h"
#include "BaaHover.h"
#include "BaaSignatureHelp.h"


class LineNumberArea;


class TEditor : public QPlainTextEdit {
    Q_OBJECT

public:
    struct Diagnostic {
        QString file;
        int line = 1;
        int column = 1;
        QString severity;
        QString message;
    };

    TEditor(QWidget* parent = nullptr);

    void setDiagnostics(const QVector<Diagnostic> &diagnostics);
    void clearDiagnostics();

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
    TSyntaxHighlighter* highlighter{};

    LineNumberArea* lineNumberArea{};

    struct FoldRegion {
        int startBlockNumber;
        int endBlockNumber;
        bool folded = false;
    };
    QVector<FoldRegion> foldRegions;

    void updateFoldRegions();
    void toggleFold(int blockNum);
    void applyEditorDecorations();
    Diagnostic diagnosticAtPosition(const QPoint &position) const;
    bool hasDiagnosticAtPosition(const QPoint &position, Diagnostic *diagnostic) const;

    // Extracted helpers
    TBracketHandler m_bracketHandler;
    TAutoSave *m_autoSave{};
    TSnippetManager m_snippetManager;

    friend class LineNumberArea;

    QCompleter* c{};
    CompletionModel *model{};
    QVector<Diagnostic> m_diagnostics;
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

private slots:
    void updateLineNumberAreaWidth();
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &rect, int dy);
signals:
    void openRequest(QString filePath);
    void completionRequested(QString filePath, int line, int character);
    void hoverRequested(QString filePath, int line, int character);
    void signatureHelpRequested(QString filePath, int line, int character);
};


class LineNumberArea : public QWidget {
public:
    LineNumberArea(TEditor* editor) : QWidget(editor), tEditor(editor) {
        this->setStyleSheet(QString(
            "QWidget {"
            "   border-left: 1px solid %1;"
            "   border-top-left-radius: 9px;"
            "   border-bottom-left-radius: 9px;"
            "}").arg(Constants::Colors::LineNumberBorder));
    }

    QSize sizeHint() const override {
        return QSize(tEditor->lineNumberAreaWidth(), 0);
    }

    void mousePressEvent(QMouseEvent* event) override {
        int y = event->position().y();
        QTextBlock block = tEditor->firstVisibleBlock();
        int top = qRound(tEditor->blockBoundingGeometry(block).translated(tEditor->contentOffset()).top());
        int height = qRound(tEditor->blockBoundingRect(block).height());

        while (block.isValid() && top <= y) {
            if (y >= top && y < top + height) {
                int blockNum = block.blockNumber();
                tEditor->toggleFold(blockNum);
                return;
            }
            block = block.next();
            top += height;
            height = qRound(tEditor->blockBoundingRect(block).height());
        }
    }


protected:
    void paintEvent(QPaintEvent* event) override {
        tEditor->lineNumberAreaPaintEvent(event);
    }


private:
    TEditor* tEditor{};
};
