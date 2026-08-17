#include "QalamWindow.h"
#include <QVBoxLayout>
#include <QApplication>
#include <QWindow>
#include <QMenuBar>
#include <QPushButton>

#if defined(Q_OS_WIN)
#include <dwmapi.h>
#include <windows.h>
#include <windowsx.h>
// Note: Libraries are linked via CMakeLists.txt (dwmapi, user32)
#endif

QalamWindow::QalamWindow(QWidget *parent) : QMainWindow(parent) {
    // 1. Setup Title Bar
    m_titleBar = new QalamTitleBar(this);
    
    connect(m_titleBar, &QalamTitleBar::minimizeClicked, this, &QMainWindow::showMinimized);
    connect(m_titleBar, &QalamTitleBar::maximizeRestoreClicked, [this]() {
        if (isMaximized()) showNormal();
        else showMaximized();
    });
    connect(m_titleBar, &QalamTitleBar::closeClicked, this, &QMainWindow::close);
    connect(m_titleBar, &QalamTitleBar::commandCenterClicked, this, &QalamWindow::commandCenterClicked);
    
    // Set menu widget to title bar (QMainWindow feature) or add to layout?
    // QMainWindow::setMenuWidget() puts it at top. Perfect.
    setMenuWidget(m_titleBar);

    connect(this, &QWidget::windowTitleChanged, m_titleBar, &QalamTitleBar::setTitle);
    
    // 2. Window Flags for Custom Frame
    setWindowFlags(Qt::Window
                   | Qt::FramelessWindowHint
                   | Qt::WindowSystemMenuHint
                   | Qt::WindowMinimizeButtonHint
                   | Qt::WindowMaximizeButtonHint);
}

void QalamWindow::setCustomMenuBar(QWidget *menu) {
    if (m_titleBar) {
        m_titleBar->addMenuBar(menu);
    }
}

bool QalamWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result) {
#if defined(Q_OS_WIN)
    if (eventType == "windows_generic_MSG") {
        MSG *msg = static_cast<MSG *>(message);
        HWND hwnd = msg->hwnd;

        switch (msg->message) {
            case WM_NCCALCSIZE: {
                if (msg->wParam == TRUE) {
                    *result = 0;
                    return true;
                }
                break;
            }
            case WM_NCHITTEST: {
                long x = GET_X_LPARAM(msg->lParam);
                long y = GET_Y_LPARAM(msg->lParam);
                
                POINT pt = {x, y};
                ScreenToClient(hwnd, &pt);

                const int borderWidth = 8;
                
                RECT rw;
                GetClientRect(hwnd, &rw);
                
                bool left = pt.x < borderWidth;
                bool right = pt.x >= rw.right - borderWidth;
                bool top = pt.y < borderWidth;
                bool bottom = pt.y >= rw.bottom - borderWidth;
                
                if (top && left) { *result = HTTOPLEFT; return true; }
                if (top && right) { *result = HTTOPRIGHT; return true; }
                if (bottom && left) { *result = HTBOTTOMLEFT; return true; }
                if (bottom && right) { *result = HTBOTTOMRIGHT; return true; }
                if (left) { *result = HTLEFT; return true; }
                if (right) { *result = HTRIGHT; return true; }
                if (bottom) { *result = HTBOTTOM; return true; }
                if (top) { *result = HTTOP; return true; }
                
                if (m_titleBar && m_titleBar->geometry().contains(QPoint(pt.x, pt.y))) {
                    QWidget *child = m_titleBar->childAt(pt.x, pt.y);
                    if (child) {
                        bool isButton = qobject_cast<QPushButton*>(child);
                        bool isMenuBar = qobject_cast<QMenuBar*>(child) || (child->parent() && qobject_cast<QMenuBar*>(child->parentWidget()));
                        if (isButton || isMenuBar) return false;
                    }
                    *result = HTCAPTION;
                    return true;
                }
                break;
            }
            case WM_SIZE: {
                if (m_titleBar) {
                    m_titleBar->setMaximizedState(windowState() & Qt::WindowMaximized);
                }
                break;
            }
        }
    }
#else
    // Non-Windows platforms: no custom native event handling needed
    Q_UNUSED(eventType);
    Q_UNUSED(message);
    Q_UNUSED(result);
#endif
    return QMainWindow::nativeEvent(eventType, message, result);
}
