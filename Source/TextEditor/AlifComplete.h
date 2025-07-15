#pragma once

#include <QObject>
#include <QListWidget>
#include <QMenu>
#include <QPlainTextEdit>
#include <QStringList>

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

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void showCompletion();
    void insertCompletion();
    void onLspCompletionReceived(const QJsonObject& response);

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

    QString getCurrentWord() const;
    void showPopup();
    void showStaticCompletion();
    inline void hidePopup();
};
