#include <QWindow>
#include <QDebug>
#include <Windows.h>

void setupTopMostWindow(QObject *rootObj, bool onTop)
{
    const auto *win = qobject_cast<QWindow*>(rootObj);
    if (nullptr != win) {
        const auto hnd = reinterpret_cast<HWND>(win->winId());
        const auto rc = SetWindowPos(hnd, onTop ? HWND_TOPMOST : HWND_NOTOPMOST,
                                     0, 0, -1, -1, SWP_NOSIZE | SWP_NOMOVE);
        if (rc) {
            qInfo() << "Set top most window";
        } else {
            qWarning() << "Cannot set window flags";
        }
    } else {
        qWarning() << "Invalid window pointer";
    }
}
