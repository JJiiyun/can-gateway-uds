#include "UdsController.h"

#include "models/AppLogModel.h"

UdsController::UdsController(QObject *parent) : QObject(parent)
{
    connect(&m_serial, &SerialDevice::portNamesChanged, this, &UdsController::portNamesChanged);
    connect(&m_serial, &SerialDevice::connectedChanged, this, &UdsController::connectedChanged);
    connect(&m_serial, &SerialDevice::statusTextChanged, this, &UdsController::statusTextChanged);
    connect(&m_serial, &SerialDevice::lineReceived, this, &UdsController::processLine);
    connect(&m_serial, &SerialDevice::commandSent, this, [this](const QString &line) {
        appendLog(QStringLiteral("TX"), line);
    });
    connect(&m_serial, &SerialDevice::serialError, this, [this](const QString &message) {
        appendLog(QStringLiteral("ERR"), message);
    });
}

void UdsController::setLogModel(AppLogModel *logModel) { m_logModel = logModel; }

QStringList UdsController::portNames() const { return m_serial.portNames(); }
bool UdsController::connected() const { return m_serial.connected(); }
QString UdsController::statusText() const { return m_serial.statusText(); }
QString UdsController::lastCommand() const { return m_lastCommand; }
QString UdsController::lastResponse() const { return m_lastResponse; }
QString UdsController::lastDid() const { return m_lastDid; }
bool UdsController::lastPositive() const { return m_lastPositive; }

void UdsController::refreshPorts() { m_serial.refreshPorts(); }
void UdsController::connectToPort(const QString &portName, int baudRate) { m_serial.connectToPort(portName, baudRate); }
void UdsController::disconnectPort() { m_serial.disconnectPort(); }

void UdsController::readVin() { sendCommand(QStringLiteral("read vin"), QStringLiteral("0xF190 VIN")); }
void UdsController::readRpm() { sendCommand(QStringLiteral("read rpm"), QStringLiteral("0xF40C RPM")); }
void UdsController::readSpeed() { sendCommand(QStringLiteral("read speed"), QStringLiteral("0xF40D Speed")); }
void UdsController::readTemp() { sendCommand(QStringLiteral("read temp"), QStringLiteral("0xF40E Coolant")); }
void UdsController::readAdas() { sendCommand(QStringLiteral("read adas"), QStringLiteral("0xF410 ADAS")); }
void UdsController::readFront() { sendCommand(QStringLiteral("read front"), QStringLiteral("0xF411 Front")); }
void UdsController::readRear() { sendCommand(QStringLiteral("read rear"), QStringLiteral("0xF412 Rear")); }
void UdsController::readFault() { sendCommand(QStringLiteral("read fault"), QStringLiteral("0xF413 Fault")); }
void UdsController::clearDtc() { sendCommand(QStringLiteral("clear dtc"), QStringLiteral("SID 0x14 Clear DTC")); }
void UdsController::sendRaw(const QString &command) { sendCommand(command.trimmed(), QStringLiteral("raw")); }

void UdsController::sendCommand(const QString &command, const QString &did)
{
    if (command.trimmed().isEmpty()) {
        return;
    }
    m_lastCommand = command.trimmed();
    if (!did.isEmpty()) {
        m_lastDid = did;
    }
    emit dataChanged();
    m_serial.sendLine(command);
}

void UdsController::processLine(const QString &line)
{
    appendLog(QStringLiteral("RX"), line);
    m_lastResponse = line;

    const QString upper = line.toUpper();
    if (upper.contains(QStringLiteral("7F")) ||
        upper.contains(QStringLiteral("NEGATIVE")) ||
        upper.contains(QStringLiteral("NRC")) ||
        upper.contains(QStringLiteral("ERROR"))) {
        m_lastPositive = false;
    } else if (upper.contains(QStringLiteral("62")) ||
               upper.contains(QStringLiteral("POSITIVE")) ||
               upper.contains(QStringLiteral("OK")) ||
               upper.contains(QStringLiteral("DID"))) {
        m_lastPositive = true;
    }

    emit dataChanged();
}

void UdsController::appendLog(const QString &direction, const QString &text)
{
    if (m_logModel) {
        m_logModel->append(QStringLiteral("UDS"), direction, text);
    }
}
