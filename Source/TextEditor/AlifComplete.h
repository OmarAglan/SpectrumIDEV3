#pragma once

#include <QObject>
#include <QListWidget>
#include <QMenu>
#include <QPlainTextEdit>
#include <QStringList>
#include "ArabicCompletionWidget.h"

// Forward declaration
class SpectrumLspClient;

class AutoComplete : public QObject
{
    Q_OBJECT
public:
    explicit AutoComplete(QPlainTextEdit* editor, QObject* parent = nullptr);

    bool isPopupVisible();

    /**
     * @brief Set LSP client for enhanced completion
     * @param lspClient Pointer to LSP client instance
     */
    void setLspClient(SpectrumLspClient* lspClient);

    /**
     * @brief Enable or disable Arabic completion system
     * @param enabled True to use Arabic completion, false for legacy system
     */
    void setArabicCompletionEnabled(bool enabled);

    /**
     * @brief Check if Arabic completion is enabled
     * @return True if Arabic completion is enabled
     */
    bool isArabicCompletionEnabled() const;

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void showCompletion();
    void insertCompletion();
    void onLspCompletionReceived(const QJsonObject& response);

    // Arabic completion slots
    void onArabicCompletionItemSelected(const ArabicCompletionItem& item);
    void onArabicCompletionItemActivated(const ArabicCompletionItem& item);
    void onArabicExampleInsertRequested(const ArabicCompletionItem& item);
    void onArabicCompletionCancelled();
    void onTypingDelayTimeout();

private:
    QPlainTextEdit* editor{};
    QWidget* popup{};
    QListWidget* listWidget{};
    QStringList keywords{};
    QMap<QString, QString> shortcuts;
    QMap<QString, QString> descriptions;
    QList<int> placeholderPositions;

    // LSP integration
    SpectrumLspClient* m_lspClient{nullptr};
    bool m_waitingForLspCompletion{false};

    // Enhanced Arabic completion
    ArabicCompletionWidget* m_arabicCompletionWidget{nullptr};
    bool m_useArabicCompletion{true};

    // Typing delay timer
    QTimer* m_typingDelayTimer{nullptr};
    static const int TYPING_DELAY_MS = 300; // 300ms delay after user stops typing

    // Inline completion preview
    QString m_inlineCompletionText;
    QTextCursor m_inlineCompletionCursor;
    bool m_showingInlineCompletion{false};

    QString getCurrentWord() const;
    void showPopup();
    void showStaticCompletion();
    inline void hidePopup();

    // Inline completion methods
    void showInlineCompletion(const QString& completion);
    void hideInlineCompletion();
    void acceptInlineCompletion();
};
