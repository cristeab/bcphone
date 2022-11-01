#include "sip_client.h"
#include "softphone.h"
#include <QTest>

class TestSipClient: public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void testInit();

private:
    SipClient *_instance = nullptr;
};

void TestSipClient::initTestCase()
{
    _instance = SipClient::instance(new Softphone());
}

void TestSipClient::testInit()
{
    QVERIFY(nullptr != _instance);
    QVERIFY(_instance->init());
}

QTEST_MAIN(TestSipClient)
#include "main.moc"
