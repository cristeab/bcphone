#include "logger.h"
#include "settings.h"
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QDebug>
#include <iostream>

namespace Logger {

#define DATE_TIME_FORMAT "dd.MM.yyyy HH:mm:ss.zzz"

/// Max number of log files
static const int _maxFiles = 9;

/// Max size of one log file. The default value is 10 MB.
static const qint64 _maxFileSize = 10 * 1024 * 1024;

static QString _logDirPath;
static QString _logFilePath;

static
bool removeLogFile(const QString &logFile)
{
    if (!QFile::exists(logFile)) {
        return true;
    }

    if (!QFile::remove(logFile)) {
        qCritical() << QString("Unable to remove log file '%1'").arg(logFile);
        return false;
    }

    return true;
}

static
bool copyLogFile(const QString &oldLogFile, const QString &newLogFile)
{
    if (!QFile::exists(oldLogFile)) {
        return true;
    }

    removeLogFile(newLogFile);

    if (!QFile::copy(oldLogFile, newLogFile)) {
        qCritical() << QString("Unable to copy old log file '%1' to '%2'")
                       .arg(oldLogFile, newLogFile);
        return false;
    }

    return true;
}

static
QString getLogFilePath(int index)
{
    return QString("%1/log-%2.txt").arg(_logDirPath).arg(index);
}

static
void backupLogFile()
{
    if (!QFile::exists(_logFilePath)) {
        return;
    }

    for (int i = _maxFiles - 1; i >= 1; --i) {
        const QString filePath = getLogFilePath(i);
        const QString newFilePath = getLogFilePath(i + 1);

        copyLogFile(filePath, newFilePath);
    }

    copyLogFile(_logFilePath, getLogFilePath(1));
}

static
QString convertTypeToString(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg:
        return QStringLiteral("DEBUG");
    case QtInfoMsg:
        return QStringLiteral("INFO");
    case QtWarningMsg:
        return QStringLiteral("WARNING");
    case QtCriticalMsg:
        return QStringLiteral("ERROR");
    case QtFatalMsg:
        return QStringLiteral("FATAL");
    default:
        return QStringLiteral("FATAL");
    }
}

static
QString getWhere(const QMessageLogContext &context)
{
    QString fileName = context.file;

    if (fileName.isEmpty() && !context.function) {
        return QString();
    }

    fileName = fileName.right(fileName.size() - fileName.lastIndexOf(QLatin1Char('/')) - 1);

    QString function = QString::fromUtf8(context.function);
    const int braceIndex = function.indexOf(QLatin1Char('('));

    if (braceIndex > 0) {
        function = function.left(braceIndex);
    }

    const int colonIndex = function.lastIndexOf(QLatin1Char(':'));

    if (colonIndex > 0) {
        function = function.right(function.size() - colonIndex - 1);
    }

    const int spaceIndex = function.lastIndexOf(QLatin1Char(' '));

    if (spaceIndex > 0) {
        function = function.right(function.size() - spaceIndex - 1);
    }

    return function.isEmpty() ? QStringLiteral("(%1:%2)")
                                .arg(fileName)
                                .arg(context.line) : QStringLiteral("(%1() %2:%3)")
                                .arg(function, fileName)
                                .arg(context.line);
}

static
QString getMessage(const QString &type,
                              const QString &when,
                              const QString &where,
                              const QString &msg)
{
    if (when.isEmpty()) {
        if (where.isEmpty()) {
            return QString("%1: %2").arg(type, msg);
        }

        return QString("%1: %2 - %3").arg(type, msg, where);
    }

    if (where.isEmpty()) {
        return QString("[%1] %2: %3").arg(when, type, msg);
    }

    return QString("[%1] %2: %3 - %4").arg(when, type, msg, where);
}

static
void logToFile(const QString &message, bool isCheckFileSize)
{
    QFile file(_logFilePath);

    if (!file.open(QFile::Append)) {
        return;
    }

    file.write(message.toUtf8());
    file.write("\r\n");

    if (isCheckFileSize && file.size() >= _maxFileSize) {
        file.write("******************** MAX FILE SIZE IS REACHED ********************");
        file.write("\r\n");

        file.close();
        backupLogFile();
        removeLogFile(_logFilePath);

        logToFile("******************** CONTINUE ********************\r\n", false);
    }
}

static
void loggingHandler(QtMsgType type,
                               const QMessageLogContext &context,
                               const QString &msg)
{
    const QString typeString = convertTypeToString(type);
    const QString where = getWhere(context);
    const QString message = getMessage(typeString,
                                       QDateTime::currentDateTime().toString(DATE_TIME_FORMAT),
                                       where,
                                       msg);

    std::clog << message.toStdString() << std::endl;

    logToFile(message, true);
}

void installLogHandler()
{
    _logDirPath = Settings::writablePath() + "/Logs";

    if (!QDir().exists(_logDirPath)) {
        if (!QDir().mkpath(_logDirPath)) {
            qCritical() << "Unable to create directory" << _logDirPath;
            return;
        }
    }

    _logFilePath = QString("%1/log.txt").arg(_logDirPath);

    backupLogFile();
    removeLogFile(_logFilePath);

    qInfo() << "Log folder path" << _logDirPath;
    qInstallMessageHandler(&loggingHandler);
}

} //Logger
