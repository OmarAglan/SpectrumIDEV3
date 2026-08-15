#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QPlainTextEdit>

class SearchPanel : public QWidget {
    Q_OBJECT

public:
    explicit SearchPanel(QWidget *parent = nullptr);
    QString getText() const;

    bool isCaseSensitive() const;
    bool isWholeWord() const;
    void setFocusToInput();

    // Set the editor that search operations apply to.
    // Called by Qalam whenever the active tab changes.
    void setEditor(QPlainTextEdit *editor);

signals:
    void closed();

private slots:
    void performFind();
    void performFindNext();
    void performFindPrev();

private:
    QLineEdit *searchInput;
    QPushButton *btnNext;
    QPushButton *btnPrev;
    QPushButton *btnClose;
    QCheckBox *checkCase;
    QCheckBox *checkWord;

    QPlainTextEdit *m_editor{};
};
