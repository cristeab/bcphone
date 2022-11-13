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
    void createClient(int index);
    void registerClient(int index);

    const QString _sipServer{"192.168.77.48"};
    const QString _unencryptedExt[2]{"0004*004", "0004*005"};
    const QString _unencryptedPwd[2]{"zyVbar-jevre5-fovtaz", "zjg44fZI3DypVxI"};

    Softphone *_softphone[2]{nullptr, nullptr};
    SipClient *_sip[2]{nullptr, nullptr};
};

void TestSipClient::createClient(int index)
{
    _softphone[index] = new Softphone();
    _sip[index] = SipClient::create(_softphone[index]);
    QVERIFY(_sip[index]->init());

    auto *settings = _softphone[index]->settings();
    settings->setSipServer(_sipServer);
    settings->setUserName(_unencryptedExt[index]);
    settings->setPassword(_unencryptedPwd[index]);
    settings->setSipTransport(Settings::SipTransport::Udp);
    settings->setMediaTransport(Settings::MediaTransport::Rtp);
}

void TestSipClient::registerClient(int index)
{
    static constexpr uint32_t maxAttempts = 3;
    QSignalSpy spy(_sip[index], &SipClient::registrationStatusChanged);
    QVERIFY(_sip[index]->registerAccount());
    auto regStatus = SipClient::RegistrationStatus::Unregistered;
    int i = 0;
    do {
        QVERIFY(spy.wait(1000));
        regStatus = qvariant_cast<SipClient::RegistrationStatus>(spy.takeFirst().at(0));
        ++i;
        if (i >= maxAttempts) {
            break;
        }
    } while (SipClient::RegistrationStatus::Registered != regStatus);
    QVERIFY(i < maxAttempts);
}

void TestSipClient::initTestCase()
{
    qRegisterMetaType<SipClient::RegistrationStatus>();
    createClient(0);
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

    createClient(1);
    registerClient(1);
}

QTEST_MAIN(TestSipClient)
#include "main.moc"
