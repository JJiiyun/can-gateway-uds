#ifndef UDSCONTROLLER_H
#define UDSCONTROLLER_H

#include <QObject>

#include "core/SerialDevice.h"

class AppLogModel;

class UdsController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList portNames READ portNames NOTIFY portNamesChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString lastCommand READ lastCommand NOTIFY dataChanged)
    Q_PROPERTY(QString lastResponse READ lastResponse NOTIFY dataChanged)
    Q_PROPERTY(QString lastDid READ lastDid NOTIFY dataChanged)
    Q_PROPERTY(bool lastPositive READ lastPositive NOTIFY dataChanged)

public:
    explicit UdsController(QObject *parent = nullptr);

    void setLogModel(AppLogModel *logModel);

    QStringList portNames() const;
    bool connected() const;
    QString statusText() const;
    QString lastCommand() const;
    QString lastResponse() const;
    QString lastDid() const;
    bool lastPositive() const;

    Q_INVOKABLE void refreshPorts();
    Q_INVOKABLE void connectToPort(const QString &portName, int baudRate = 115200);
    Q_INVOKABLE void disconnectPort();
    Q_INVOKABLE void readVin();
    Q_INVOKABLE void readRpm();
    Q_INVOKABLE void readSpeed();
    Q_INVOKABLE void readTemp();
    Q_INVOKABLE void readAdas();
    Q_INVOKABLE void readFront();
    Q_INVOKABLE void readRear();
    Q_INVOKABLE void readFault();
    Q_INVOKABLE void clearDtc();
    Q_INVOKABLE void sendRaw(const QString &command);

signals:
    void portNamesChanged();
    void connectedChanged();
    void statusTextChanged();
    void dataChanged();

private:
    void sendCommand(const QString &command, const QString &did = QString());
    void processLine(const QString &line);
    void appendLog(const QString &direction, const QString &text);

    SerialDevice m_serial;
    AppLogModel *m_logModel = nullptr;
    QString m_lastCommand = QStringLiteral("-");
    QString m_lastResponse = QStringLiteral("-");
    QString m_lastDid = QStringLiteral("-");
    bool m_lastPositive = false;
};

#endif // UDSCONTROLLER_H
