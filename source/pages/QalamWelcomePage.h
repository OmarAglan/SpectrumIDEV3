#pragma once

#include <QWidget>

class QCheckBox;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QSettings;

class QalamWelcomePage : public QWidget
{
    Q_OBJECT

public:
    explicit QalamWelcomePage(QWidget *parent = nullptr,
                              const QString &settingsFilePath = QString());
    ~QalamWelcomePage() override = default;

    void refreshRecents();

signals:
    void newFileRequested();
    void openFileRequested();
    void openFolderRequested();
    void cloneRepoRequested();
    void recentPathRequested(const QString &path);
    void reopenLastProjectRequested(const QString &path);
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
    void populateRecentList(QListWidget *list, const QStringList &paths,
                            bool projects);
    void showEmptyRecentsState(QListWidget *list, bool projects);
    void removeFromRecents(const QString &path);
    QSettings *createSettings() const;

    bool loadShowOnStartup() const;
    void saveShowOnStartup(bool show);

private:
    QListWidget *m_recentList{};
    QListWidget *m_recentFilesList{};
    QPushButton *m_reopenLastProjectButton{};
    QPushButton *m_clearRecentsBtn{};
    QCheckBox *m_showOnStartupCheck{};
    QString m_settingsFilePath;
};

