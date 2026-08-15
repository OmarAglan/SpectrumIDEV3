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
    void paintEvent(QPaintEvent *event) override;
    // Mouse handling for dragging is handled by parent QalamWindow logic usually,
    // or we can bubble it up. Using WM_NCHITTEST is better.

private:
    QLabel *m_iconLabel;
    QLabel *m_titleLabel;
    QPushButton *m_commandCenterBtn;

    QPushButton *m_minimizeBtn;
    QPushButton *m_maximizeBtn;
    QPushButton *m_closeBtn;

    void setupUi();
    QPushButton* createCaptionButton(const QString &iconPath, const QString &objName);
};
