#include <QWindow>
#include <QPainter>
#include <QGuiApplication>
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

static
QIcon generateIconBadgeNumber(int num, const QString& imageFileName)
{
    QPixmap img(imageFileName);
    if (img.isNull()) {
        qWarning() << "Cannot open" << imageFileName;
        return QIcon();
    }
    if (0 == num) {
        return img;
    }

    QPainter painter;
    painter.begin(&img);

    QPen pen(Qt::white);
    painter.setPen(pen);

    QFont font;
    font.setFamily("Times");
    font.setBold(true);
    static constexpr int fontPointSize = 220;
    font.setPointSize(fontPointSize);
    painter.setFont(font);

    auto numDigits = [](int num) {
        int d = 0;
        if (num < 10) {
            d = 1;
        } else if (num < 100) {
            d = 2;
        } else {
            d = 3;
        }
        return d;
    };

    painter.drawText(img.width() - numDigits(num) * fontPointSize, fontPointSize + 20, QString::number(num));

    painter.end();

    return img;
}

void setApplicationIconBadgeNumber(int num)
{
    qDebug() << "App badge no" << num;
    const auto icon = generateIconBadgeNumber(num, ":/img/logo.png");
    QGuiApplication::setWindowIcon(icon);
}
