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
    void testMakeCall();

private:
    void createSecondClient();

    const QString _sipServer{"192.168.77.48"};
    const QString _unencryptedExt[2]{"0004*004", "0004*005"};
    const QString _unencryptedPwd[2]{"zyVbar-jevre5-fovtaz", "zjg44fZI3DypVxI"};

    Softphone *_softphone[2]{nullptr, nullptr};
    SipClient *_sip[2]{nullptr, nullptr};
};

void TestSipClient::createSecondClient()
{
    /*_softphone[1] = new Softphone();
    _sip[1] = SipClient::instance(_softphone[1]);
    QVERIFY(_sip[1]->init());

    auto *settings = _softphone[1]->settings();
    settings->setSipServer(_sipServer);
    settings->setUserName(_unencryptedExt[1]);
    settings->setPassword(_unencryptedPwd[1]);
    settings->setSipTransport(Settings::SipTransport::Udp);
    settings->setMediaTransport(Settings::MediaTransport::Rtp);

    QSignalSpy spy(_sip[1], &SipClient::registrationStatusChanged);
    QVERIFY(_sip[1]->registerAccount());
    QVERIFY(spy.wait(1000));

    auto regStatus = qvariant_cast<SipClient::RegistrationStatus>(spy.takeFirst().at(0));
    QCOMPARE(regStatus, SipClient::RegistrationStatus::Registered);*/
}

void TestSipClient::initTestCase()
{
    qRegisterMetaType<SipClient::RegistrationStatus>();
    _softphone[0] = new Softphone();
    _sip[0] = SipClient::create(_softphone[0]);
    QVERIFY(_sip[0]->init());

    auto *settings = _softphone[0]->settings();
    settings->setSipServer(_sipServer);
    settings->setUserName(_unencryptedExt[0]);
    settings->setPassword(_unencryptedPwd[0]);
    settings->setSipTransport(Settings::SipTransport::Udp);
    settings->setMediaTransport(Settings::MediaTransport::Rtp);
}

void TestSipClient::cleanupTestCase()
{
    QElapsedTimer timer;
    timer.start();
    _sip[0]->release();
    qDebug() << "Cleanup took" << timer.elapsed() / 1000.0 << "seconds";
}

void TestSipClient::testRegisterAccount()
{
    QSignalSpy spy(_sip[0], &SipClient::registrationStatusChanged);

    QVERIFY(_sip[0]->registerAccount());

    QCOMPARE(spy.count(), 1);
    auto regStatus = qvariant_cast<SipClient::RegistrationStatus>(spy.takeFirst().at(0));
    QCOMPARE(regStatus, SipClient::RegistrationStatus::Trying);

    QVERIFY(spy.wait(1000));
    QCOMPARE(spy.count(), 1);
    regStatus = qvariant_cast<SipClient::RegistrationStatus>(spy.takeFirst().at(0));
    QCOMPARE(regStatus, SipClient::RegistrationStatus::Registered);

    _sip[0]->manuallyRegister();
    QVERIFY(spy.wait(1000));
    QCOMPARE(spy.count(), 1);
    regStatus = qvariant_cast<SipClient::RegistrationStatus>(spy.takeFirst().at(0));
    QCOMPARE(regStatus, SipClient::RegistrationStatus::Registered);

    QVERIFY(_sip[0]->unregisterAccount());
}

void TestSipClient::testMakeCall()
{
    QSignalSpy spy(_sip[0], &SipClient::registrationStatusChanged);
    QVERIFY(_sip[0]->registerAccount());
    QVERIFY(spy.wait(1000));
    auto regStatus = qvariant_cast<SipClient::RegistrationStatus>(spy.takeFirst().at(0));
    QCOMPARE(regStatus, SipClient::RegistrationStatus::Registered);

    QSignalSpy callingSpy(_sip[0], &SipClient::calling);
    QSignalSpy confirmedSpy(_sip[0], &SipClient::confirmed);
    QSignalSpy disconnectedSpy(_sip[0], &SipClient::disconnected);

    QVERIFY(_sip[0]->makeCall(_unencryptedExt[1].split('*').back()));

    //QVERIFY(callingSpy.wait(1000));
    //QVERIFY(confirmedSpy.wait(1000));
    QVERIFY(disconnectedSpy.wait(1000));

    createSecondClient();
}

QTEST_MAIN(TestSipClient)
#include "main.moc"
