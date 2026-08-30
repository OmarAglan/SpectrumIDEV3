#include "QalamWindow.h"
#include <QVBoxLayout>
#include <QApplication>
#include <QAbstractButton>
#include <QWindow>
#include <QMenuBar>

#if defined(Q_OS_WIN)
#include <dwmapi.h>
#include <windows.h>
#include <windowsx.h>
// Note: Libraries are linked via CMakeLists.txt (dwmapi, user32)
#endif

namespace {

bool isInteractiveTitleBarWidget(QWidget *widget, QWidget *titleBar)
{
    for (QWidget *candidate = widget;
         candidate and candidate != titleBar;
         candidate = candidate->parentWidget()) {
        if (qobject_cast<QAbstractButton*>(candidate) or
            qobject_cast<QMenuBar*>(candidate)) {
            return true;
        }
    }
    return false;
}

bool isMaximizeButtonWidget(QWidget *widget, QWidget *titleBar)
{
    for (QWidget *candidate = widget;
         candidate and candidate != titleBar;
         candidate = candidate->parentWidget()) {
        if (candidate->objectName() == QStringLiteral("maximizeButton"))
            return true;
    }
    return false;
}

#if defined(Q_OS_WIN)
int nativeResizeBorderWidth(HWND hwnd)
{
    UINT dpi = GetDpiForWindow(hwnd);
    if (dpi == 0) dpi = USER_DEFAULT_SCREEN_DPI;
    const int frame = GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi);
    const int padding = GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
    // Preserve an approachable edge even with themes that report a very
    // narrow padded frame.
    return qMax(MulDiv(10, static_cast<int>(dpi), 96), frame + padding);
}
#endif

}

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
    Qt::WindowFlags flags = Qt::Window
        | Qt::WindowTitleHint
        | Qt::WindowSystemMenuHint
        | Qt::WindowMinimizeButtonHint
        | Qt::WindowMaximizeButtonHint
        | Qt::WindowCloseButtonHint;
#if not defined(Q_OS_WIN)
    // Windows retains its real resizable frame and caption behavior below;
    // WM_NCCALCSIZE turns that frame into client space for Qalam's own title
    // bar.  Qt::FramelessWindowHint removes WS_THICKFRAME and consequently
    // disables native resize/Snap behavior on some Windows builds.
    flags |= Qt::FramelessWindowHint;
#endif
    setWindowFlags(flags);
}

void QalamWindow::setCustomMenuBar(QWidget *menu) {
    if (m_titleBar) {
        m_titleBar->addMenuBar(menu);
    }
}

bool QalamWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result) {
#if defined(Q_OS_WIN)
    if (eventType == "windows_generic_MSG" or
        eventType == "windows_dispatcher_MSG") {
        MSG *msg = static_cast<MSG *>(message);
        HWND hwnd = msg->hwnd;

        // Microsoft requires custom frames to offer non-client hit testing to
        // DWM first. This preserves native caption-button hover behavior and
        // the Windows 11 Snap-layout flyout; Qalam handles only regions that
        // DWM leaves unresolved.
        if (msg->message == WM_NCHITTEST
            or msg->message == WM_NCMOUSELEAVE) {
            LRESULT dwmResult = 0;
            if (DwmDefWindowProc(hwnd, msg->message, msg->wParam,
                                 msg->lParam, &dwmResult)) {
                *result = static_cast<qintptr>(dwmResult);
                return true;
            }
        }

        switch (msg->message) {
            case WM_GETMINMAXINFO: {
                // Respect the usable rectangle of the monitor selected by
                // Windows.  Without this, a frameless window can retain stale
                // geometry after Snap or a monitor/DPI transition.
                auto *limits = reinterpret_cast<MINMAXINFO*>(msg->lParam);
                const HMONITOR monitor = MonitorFromWindow(
                    hwnd, MONITOR_DEFAULTTONEAREST);
                MONITORINFO monitorInfo{};
                monitorInfo.cbSize = sizeof(monitorInfo);
                if (monitor and GetMonitorInfoW(monitor, &monitorInfo)) {
                    const RECT work = monitorInfo.rcWork;
                    const RECT bounds = monitorInfo.rcMonitor;
                    limits->ptMaxPosition.x = work.left - bounds.left;
                    limits->ptMaxPosition.y = work.top - bounds.top;
                    limits->ptMaxSize.x = work.right - work.left;
                    limits->ptMaxSize.y = work.bottom - work.top;
                    // Qt reports widget sizes in device-independent pixels,
                    // while MINMAXINFO uses native pixels.  Supplying the
                    // minimum here keeps Windows Snap/resize responsive on
                    // mixed-DPI monitors instead of oscillating around Qt's
                    // minimum-size correction.
                    const UINT dpi = GetDpiForWindow(hwnd);
                    const int scaleDpi = dpi > 0 ? static_cast<int>(dpi) : 96;
                    limits->ptMinTrackSize.x = MulDiv(
                        minimumWidth(), scaleDpi, 96);
                    limits->ptMinTrackSize.y = MulDiv(
                        minimumHeight(), scaleDpi, 96);
                    *result = 0;
                    return true;
                }
                break;
            }
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

                const int borderWidth = nativeResizeBorderWidth(hwnd);
                
                RECT rw;
                GetClientRect(hwnd, &rw);
                
                const bool resizable = not IsZoomed(hwnd);
                bool left = resizable and pt.x < borderWidth;
                bool right = resizable and pt.x >= rw.right - borderWidth;
                bool top = resizable and pt.y < borderWidth;
                bool bottom = resizable and pt.y >= rw.bottom - borderWidth;
                
                if (top && left) { *result = HTTOPLEFT; return true; }
                if (top && right) { *result = HTTOPRIGHT; return true; }
                if (bottom && left) { *result = HTBOTTOMLEFT; return true; }
                if (bottom && right) { *result = HTBOTTOMRIGHT; return true; }
                if (left) { *result = HTLEFT; return true; }
                if (right) { *result = HTRIGHT; return true; }
                if (bottom) { *result = HTBOTTOM; return true; }
                if (top) { *result = HTTOP; return true; }
                
                if (m_titleBar
                    and m_titleBar->geometry().contains(QPoint(pt.x, pt.y))) {
                    const QPoint titlePoint = m_titleBar->mapFrom(
                        this, QPoint(pt.x, pt.y));
                    QWidget *child = m_titleBar->childAt(titlePoint);
                    if (isMaximizeButtonWidget(child, m_titleBar)) {
                        // Windows 11 uses HTMAXBUTTON for the native Snap
                        // layout flyout.  WM_NCLBUTTONUP below performs the
                        // actual maximize/restore action.
                        *result = HTMAXBUTTON;
                        return true;
                    }
                    if (isInteractiveTitleBarWidget(child, m_titleBar))
                        return false;
                    *result = HTCAPTION;
                    return true;
                }
                break;
            }
            case WM_NCLBUTTONUP: {
                if (msg->wParam == HTMAXBUTTON) {
                    if (IsZoomed(hwnd)) showNormal();
                    else showMaximized();
                    *result = 0;
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
