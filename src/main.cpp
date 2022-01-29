#include "softphone.h"
#include "config.h"
#include "logger.h"
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <memory>
#include <QDebug>

//TODO: QML debugging
//#include <QQmlDebuggingEnabler>
//QQmlDebuggingEnabler enabler;

int main(int argc, char *argv[])
{
    if (1 < argc) {
        if (0 == qstrcmp("clear", argv[1])) {
            //calling with "clear" as argument means that the app is uninstalled
            Settings::uninstallClear();
            return EXIT_SUCCESS;
        }
    }
    //main application
    QApplication app(argc, argv);

    QGuiApplication::setOrganizationName(ORG_NAME);
    QGuiApplication::setApplicationName(APP_NAME);
    QGuiApplication::setApplicationVersion(APP_VERSION);

    qSetMessagePattern("%{appname} [%{threadid}] [%{type}] %{message} (%{file}:%{line})");
    Logger::installLogHandler();

    //must be instantiated before QML engine
    std::unique_ptr<Softphone> softphone(new Softphone());

    QQmlApplicationEngine engine;
    //set properties
    QQmlContext *context = engine.rootContext();//registered properties are available to all components
    if (nullptr != context) {
        qDebug() << "*** Application started ***";
        context->setContextProperty(softphone->objectName(), softphone.get());
    } else {
        qDebug() << "Cannot get root context";
        return EXIT_FAILURE;
    }

    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    QList<QObject*> rootObj = engine.rootObjects();
    if (!rootObj.isEmpty() && (nullptr != rootObj[0])) {
        softphone->setMainForm(rootObj[0]);
    }

    QGuiApplication::setQuitOnLastWindowClosed(false);
    auto *settings = softphone->settings();
    QObject::connect(&app, &QGuiApplication::lastWindowClosed, settings, &Settings::save);

    return QGuiApplication::exec();
}
