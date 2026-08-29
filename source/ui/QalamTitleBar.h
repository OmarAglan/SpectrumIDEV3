#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>

class QMenuBar;

class QalamTitleBar : public QWidget {
    Q_OBJECT
public:
    explicit QalamTitleBar(QWidget *parent = nullptr);

    void setTitle(const QString &title);
    void setMaximizedState(bool maximized);
    void addMenuBar(QWidget *menu);

signals:
    void minimizeClicked();
    void maximizeRestoreClicked();
    void closeClicked();
    void commandCenterClicked();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QLabel *m_iconLabel;
    QLabel *m_titleLabel;
    QPushButton *m_commandCenterBtn;

    QPushButton *m_minimizeBtn;
    QPushButton *m_maximizeBtn;
    QPushButton *m_closeBtn;
    QHBoxLayout *m_rightLayout{};
    QWidget *m_menuWidget{};
    int m_menuPreferredWidth{};
    int m_rightContentWidth{};

    void setupUi();
    bool isInteractiveTitleBarChild(const QObject *object) const;
    void updateCommandCenterWidth();
    QPushButton* createCaptionButton(const QString &iconPath, const QString &objName);
};
