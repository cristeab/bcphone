#include "sip_client.h"
#include "softphone.h"
#include <QSignalSpy>
#include <QTest>
#include <QElapsedTimer>

class TestSipClient: public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();

    void testRegisterAccount();

private:
    const QString _sipServer{"192.168.77.48"};
    const QString _unencryptedExt{"0004*004"};
    const QString _unencryptedPwd{"zyVbar-jevre5-fovtaz"};

    Softphone *_softphone{nullptr};
    SipClient *_sip{nullptr};
};

void TestSipClient::initTestCase()
{
    qRegisterMetaType<SipClient::RegistrationStatus>();
    _softphone = new Softphone();
    _sip = SipClient::instance(_softphone);
    QVERIFY(_sip->init());
}

void TestSipClient::cleanupTestCase()
{
    QElapsedTimer timer;
    timer.start();
    _sip->release();
    qDebug() << "Cleanup took" << timer.elapsed() / 1000.0 << "seconds";
}

void TestSipClient::testRegisterAccount()
{
    auto *settings = _softphone->settings();
    settings->setSipServer(_sipServer);
    settings->setUserName(_unencryptedExt);
    settings->setPassword(_unencryptedPwd);
    settings->setSipTransport(Settings::SipTransport::Udp);
    settings->setMediaTransport(Settings::MediaTransport::Rtp);

    QSignalSpy spy(_sip, &SipClient::registrationStatusChanged);

    QVERIFY(_sip->registerAccount());

    QCOMPARE(spy.count(), 1);
    auto regStatus = qvariant_cast<SipClient::RegistrationStatus>(spy.takeFirst().at(0));
    QCOMPARE(regStatus, SipClient::RegistrationStatus::Trying);

    QVERIFY(spy.wait(1000));
    QCOMPARE(spy.count(), 1);
    regStatus = qvariant_cast<SipClient::RegistrationStatus>(spy.takeFirst().at(0));
    QCOMPARE(regStatus, SipClient::RegistrationStatus::Registered);

    _sip->manuallyRegister();
    QVERIFY(spy.wait(1000));
    QCOMPARE(spy.count(), 1);
    regStatus = qvariant_cast<SipClient::RegistrationStatus>(spy.takeFirst().at(0));
    QCOMPARE(regStatus, SipClient::RegistrationStatus::Registered);

    QVERIFY(_sip->unregisterAccount());
}

QTEST_MAIN(TestSipClient)
#include "main.moc"
