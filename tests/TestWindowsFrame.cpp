#include "QalamTitleBar.h"
#include "QalamWindow.h"
#include "Constants.h"

#include <QApplication>
#include <QPushButton>
#include <QTest>

#include <windows.h>
#include <windowsx.h>

namespace {

LRESULT hitTest(HWND hwnd, QWidget *window, const QPoint &clientPoint)
{
    const QPoint screenPoint = window->mapToGlobal(clientPoint);
    return SendMessageW(hwnd, WM_NCHITTEST, 0,
                        MAKELPARAM(screenPoint.x(), screenPoint.y()));
}

}

class TestWindowsFrame final : public QObject
{
    Q_OBJECT

private slots:
    void exposesNativeMoveResizeAndSnapContracts();
    void nativeMaximizeButtonTogglesWindowState();
};

void TestWindowsFrame::exposesNativeMoveResizeAndSnapContracts()
{
    QalamWindow window;
    window.setMinimumSize(Constants::Layout::WindowMinWidth,
                          Constants::Layout::WindowMinHeight);
    window.resize(1100, 720);
    window.show();
    QApplication::processEvents();

    const HWND hwnd = reinterpret_cast<HWND>(window.winId());
    QVERIFY(hwnd);
    const LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
    QVERIFY2(style & WS_THICKFRAME,
             "Qalam must retain the native resizable frame on Windows");
    QVERIFY2(not (style & WS_CAPTION),
             "DWM must not draw a second caption over Qalam's title bar");
    QVERIFY2(style & WS_MAXIMIZEBOX,
             "Qalam must retain the native maximize/Snap contract");
    QVERIFY(style & WS_MINIMIZEBOX);
    QVERIFY(style & WS_SYSMENU);

    MINMAXINFO limits{};
    SendMessageW(hwnd, WM_GETMINMAXINFO, 0,
                 reinterpret_cast<LPARAM>(&limits));
    const UINT dpi = qMax<UINT>(GetDpiForWindow(hwnd), 96);
    QVERIFY(limits.ptMinTrackSize.x
            <= MulDiv(500, static_cast<int>(dpi), 96));
    QCOMPARE(limits.ptMinTrackSize.x,
             static_cast<LONG>(MulDiv(Constants::Layout::WindowMinWidth,
                                      static_cast<int>(dpi), 96)));

    auto *titleBar = window.findChild<QalamTitleBar*>();
    QVERIFY(titleBar);
    auto *commandCenter = titleBar->findChild<QPushButton*>(
        QStringLiteral("commandCenterButton"));
    auto *maximizeButton = titleBar->findChild<QPushButton*>(
        QStringLiteral("maximizeButton"));
    QVERIFY(commandCenter);
    QVERIFY(maximizeButton);

    QCOMPARE(hitTest(hwnd, &window, QPoint(1, window.height() / 2)),
             static_cast<LRESULT>(HTLEFT));
    QCOMPARE(hitTest(hwnd, &window,
                     titleBar->mapTo(&window,
                                     QPoint(120, titleBar->height() / 2))),
             static_cast<LRESULT>(HTCAPTION));
    QCOMPARE(hitTest(hwnd, &window,
                     maximizeButton->mapTo(&window,
                                           maximizeButton->rect().center())),
             static_cast<LRESULT>(HTMAXBUTTON));

    // A command-centre click remains a normal client click; its custom button
    // switches to QWindow::startSystemMove only after the drag threshold.
    const LRESULT commandHit = hitTest(
        hwnd, &window,
        commandCenter->mapTo(&window, commandCenter->rect().center()));
    QVERIFY(commandHit != HTCAPTION);
    QVERIFY(commandHit != HTLEFT);
    QVERIFY(commandHit != HTRIGHT);
}

void TestWindowsFrame::nativeMaximizeButtonTogglesWindowState()
{
    QalamWindow window;
    window.setMinimumSize(Constants::Layout::WindowMinWidth,
                          Constants::Layout::WindowMinHeight);
    window.resize(1000, 680);
    window.show();
    QApplication::processEvents();
    const HWND hwnd = reinterpret_cast<HWND>(window.winId());
    QVERIFY(hwnd);

    SendMessageW(hwnd, WM_NCLBUTTONUP, HTMAXBUTTON, 0);
    QTRY_VERIFY(window.isMaximized());

    SendMessageW(hwnd, WM_NCLBUTTONUP, HTMAXBUTTON, 0);
    QTRY_VERIFY(not window.isMaximized());
    QVERIFY(not (GetWindowLongPtrW(hwnd, GWL_STYLE) & WS_CAPTION));
}

QTEST_MAIN(TestWindowsFrame)
#include "TestWindowsFrame.moc"
