#include "SerialDevice.h"

#include <QSerialPortInfo>

SerialDevice::SerialDevice(QObject *parent) : QObject(parent)
{
    connect(&m_serial, &QSerialPort::readyRead, this, &SerialDevice::onReadyRead);
    connect(&m_serial, &QSerialPort::errorOccurred, this, &SerialDevice::onErrorOccurred);
    refreshPorts();
    setStatusText(m_portNames.isEmpty() ? QStringLiteral("No serial ports found")
                                         : QStringLiteral("Select a serial port"));
}

QStringList SerialDevice::portNames() const
{
    return m_portNames;
}

bool SerialDevice::connected() const
{
    return m_serial.isOpen();
}

QString SerialDevice::statusText() const
{
    return m_statusText;
}

QString SerialDevice::portName() const
{
    return m_portName;
}

void SerialDevice::refreshPorts()
{
    QStringList names;
    const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &port : ports) {
        names.append(port.portName());
    }
    names.removeDuplicates();
    names.sort();

    if (names != m_portNames) {
        m_portNames = names;
        emit portNamesChanged();
    }

    if (m_portNames.isEmpty() && !m_serial.isOpen()) {
        setStatusText(QStringLiteral("No serial ports found"));
    }
}

void SerialDevice::connectToPort(const QString &portName, int baudRate)
{
    if (m_serial.isOpen()) {
        m_serial.close();
        emit connectedChanged();
    }

    if (portName.trimmed().isEmpty()) {
        setStatusText(QStringLiteral("Select a serial port first"));
        return;
    }

    m_serial.setPortName(portName.trimmed());
    m_serial.setBaudRate(baudRate);
    m_serial.setDataBits(QSerialPort::Data8);
    m_serial.setParity(QSerialPort::NoParity);
    m_serial.setStopBits(QSerialPort::OneStop);
    m_serial.setFlowControl(QSerialPort::NoFlowControl);

    if (!m_serial.open(QIODevice::ReadWrite)) {
        setStatusText(QStringLiteral("Open failed: %1").arg(m_serial.errorString()));
        emit serialError(m_statusText);
        emit connectedChanged();
        return;
    }

    m_rxBuffer.clear();
    m_portName = portName.trimmed();
    setStatusText(QStringLiteral("Connected to %1 @ %2").arg(m_portName).arg(baudRate));
    emit connectedChanged();
}

void SerialDevice::disconnectPort()
{
    if (m_serial.isOpen()) {
        m_serial.close();
    }
    m_rxBuffer.clear();
    setStatusText(QStringLiteral("Disconnected"));
    emit connectedChanged();
}

void SerialDevice::sendLine(const QString &line)
{
    const QString trimmed = line.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }

    if (!m_serial.isOpen()) {
        const QString message = QStringLiteral("Serial port is not connected: %1").arg(trimmed);
        setStatusText(message);
        emit serialError(message);
        return;
    }

    QByteArray payload = trimmed.toUtf8();
    payload.append("\r\n");
    const qint64 written = m_serial.write(payload);
    if (written < 0) {
        const QString message = QStringLiteral("Write failed: %1").arg(m_serial.errorString());
        setStatusText(message);
        emit serialError(message);
        return;
    }
    emit commandSent(trimmed);
}

void SerialDevice::onReadyRead()
{
    m_rxBuffer.append(m_serial.readAll());
    while (true) {
        const int newline = m_rxBuffer.indexOf('\n');
        if (newline < 0) {
            break;
        }

        QByteArray rawLine = m_rxBuffer.left(newline);
        m_rxBuffer.remove(0, newline + 1);
        rawLine.replace("\r", "");
        const QString line = QString::fromUtf8(rawLine).trimmed();
        if (!line.isEmpty()) {
            emit lineReceived(line);
        }
    }
}

void SerialDevice::onErrorOccurred(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) {
        return;
    }

    if (error == QSerialPort::ResourceError) {
        const QString message = QStringLiteral("Serial disconnected: %1").arg(m_serial.errorString());
        setStatusText(message);
        m_serial.close();
        emit serialError(message);
        emit connectedChanged();
    }
}

void SerialDevice::setStatusText(const QString &text)
{
    if (m_statusText == text) {
        return;
    }
    m_statusText = text;
    emit statusTextChanged();
}
