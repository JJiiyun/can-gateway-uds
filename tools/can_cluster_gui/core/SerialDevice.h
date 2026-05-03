#ifndef SERIALDEVICE_H
#define SERIALDEVICE_H

#include <QObject>
#include <QSerialPort>
#include <QStringList>

class SerialDevice : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList portNames READ portNames NOTIFY portNamesChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString portName READ portName NOTIFY connectedChanged)

public:
    explicit SerialDevice(QObject *parent = nullptr);

    QStringList portNames() const;
    bool connected() const;
    QString statusText() const;
    QString portName() const;

    Q_INVOKABLE void refreshPorts();
    Q_INVOKABLE void connectToPort(const QString &portName, int baudRate = 115200);
    Q_INVOKABLE void disconnectPort();
    Q_INVOKABLE void sendLine(const QString &line);

signals:
    void portNamesChanged();
    void connectedChanged();
    void statusTextChanged();
    void lineReceived(const QString &line);
    void commandSent(const QString &line);
    void serialError(const QString &message);

private slots:
    void onReadyRead();
    void onErrorOccurred(QSerialPort::SerialPortError error);

private:
    void setStatusText(const QString &text);

    QSerialPort m_serial;
    QByteArray m_rxBuffer;
    QStringList m_portNames;
    QString m_statusText;
    QString m_portName;
};

#endif // SERIALDEVICE_H
