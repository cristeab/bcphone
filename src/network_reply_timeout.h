#pragma once

#include <QNetworkReply>
#include <QBasicTimer>
#include <QTimerEvent>

class NetworkReplyTimeout : public QObject {
    Q_OBJECT
    QBasicTimer m_timer;
public:
    NetworkReplyTimeout(QNetworkReply* reply, const int timeout) : QObject(reply) {
        Q_ASSERT(reply);
        if (reply && reply->isRunning()) {
            m_timer.start(timeout, this);
        }
    }
    static void set(QNetworkReply* reply, const int timeout) {
        if (nullptr != reply) {
            new NetworkReplyTimeout(reply, timeout);
        }
    }
protected:
    void timerEvent(QTimerEvent * ev) {
        if (!m_timer.isActive() || ev->timerId() != m_timer.timerId()) {
            return;
        }
        auto reply = static_cast<QNetworkReply*>(parent());
        if (reply->isRunning()) {
            //qWarning() << "Reply timeout" << reply;
            reply->close();//will emit finished signal
        }
        m_timer.stop();
    }
};
