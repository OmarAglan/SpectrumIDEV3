#pragma once

#include <QWidget>

class QCheckBox;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QPushButton;

class QalamWelcomePage : public QWidget
{
    Q_OBJECT

public:
    explicit QalamWelcomePage(QWidget *parent = nullptr);
    ~QalamWelcomePage() override = default;

    void refreshRecents();

signals:
    void newFileRequested();
    void openFileRequested();
    void openFolderRequested();
    void cloneRepoRequested();
    void recentFileRequested(const QString &path);
    void showWelcomeOnStartupChanged(bool show);

private slots:
    void onRecentItemActivated(QListWidgetItem *item);
    void onClearRecentsRequested();
    void onShowOnStartupToggled(bool show);

private:
    void setupUi();
    void applyStyles();

    QPushButton *createActionButton(const QString &iconPath, const QString &text);
    void populateRecents();
    void showEmptyRecentsState();
    void removeFromRecents(const QString &path);

    bool loadShowOnStartup() const;
    void saveShowOnStartup(bool show);

private:
    QListWidget *m_recentList{};
    QPushButton *m_clearRecentsBtn{};
    QCheckBox *m_showOnStartupCheck{};
};

