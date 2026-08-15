#pragma once

#include <QDialog>
#include <QString>
#include <QVector>

class QLineEdit;
class QListWidget;
class QListWidgetItem;

/**
 * @brief Lightweight VS Code-like command palette / quick-open dialog.
 *
 * The widget is intentionally generic: callers provide a list of command/file
 * rows and receive the selected id back through commandActivated().
 */
class QalamCommandPalette : public QDialog
{
    Q_OBJECT

public:
    struct Entry {
        QString id;
        QString title;
        QString subtitle;
        QString shortcut;
        QString payload;
    };

    explicit QalamCommandPalette(QWidget *parent = nullptr);

    void setPlaceholderText(const QString &text);
    void setEmptyText(const QString &text);
    void setEntries(const QVector<Entry> &entries);
    void setInitialQuery(const QString &query);

signals:
    void commandActivated(const QString &id);
    void entryActivated(const QString &id, const QString &payload);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void filterEntries();
    void activateCurrentItem();
    void activateItem(QListWidgetItem *item);

private:
    void setupUi();
    void applyStyles();
    void addVisibleEntry(const Entry &entry);
    bool matches(const Entry &entry, const QString &query) const;

    QLineEdit *m_input{};
    QListWidget *m_list{};
    QVector<Entry> m_entries;
    QString m_emptyText = "لا توجد نتائج";
};
