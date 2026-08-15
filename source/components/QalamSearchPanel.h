#pragma once

#include <QPointer>
#include <QStringList>
#include <QVector>
#include <QWidget>

class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QalamEditor;
class QRegularExpression;
class QHideEvent;
class QShowEvent;

class QalamSearchPanel : public QWidget {
    Q_OBJECT

public:
    explicit QalamSearchPanel(QWidget *parent = nullptr);

    QString searchText() const;
    QString replacementText() const;
    bool isCaseSensitive() const;
    bool isWholeWord() const;
    bool isRegex() const;
    bool hasValidPattern() const { return m_patternValid; }
    int matchCount() const { return m_matches.size(); }
    int currentMatchNumber() const;

    void setEditor(QalamEditor *editor);
    void setFocusToInput();
    void clearHighlights();

signals:
    void closed();

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private slots:
    void performFind();
    void performFindNext();
    void performFindPrevious();
    void performReplace();
    void performReplaceAll();
    void refreshAfterEditorChange();

private:
    struct Match {
        int start{};
        int length{};
        QStringList captures;
    };

    QRegularExpression buildPattern() const;
    void refreshMatches(int preferredPosition, bool selectCurrent);
    void selectMatch(int index);
    void publishHighlights();
    void updateCountLabel();
    void updatePatternState();
    QString replacementFor(const Match &match) const;

    QLineEdit *m_searchInput{};
    QLineEdit *m_replaceInput{};
    QPushButton *m_nextButton{};
    QPushButton *m_previousButton{};
    QPushButton *m_replaceButton{};
    QPushButton *m_replaceAllButton{};
    QPushButton *m_closeButton{};
    QCheckBox *m_caseCheck{};
    QCheckBox *m_wordCheck{};
    QCheckBox *m_regexCheck{};
    QLabel *m_countLabel{};

    QPointer<QalamEditor> m_editor;
    QVector<Match> m_matches;
    int m_currentMatchIndex{-1};
    bool m_patternValid{true};
    bool m_replacing{};
    bool m_searchActive{};
};
