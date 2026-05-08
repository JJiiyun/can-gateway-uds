#include "GatewayController.h"

#include <QDateTime>
#include <QRegularExpression>

#include "models/AppLogModel.h"

GatewayController::GatewayController(QObject *parent) : QObject(parent)
{
    connect(&m_serial, &SerialDevice::portNamesChanged, this, &GatewayController::portNamesChanged);
    connect(&m_serial, &SerialDevice::connectedChanged, this, &GatewayController::connectedChanged);
    connect(&m_serial, &SerialDevice::statusTextChanged, this, &GatewayController::statusTextChanged);
    connect(&m_serial, &SerialDevice::lineReceived, this, &GatewayController::processLine);
    connect(&m_serial, &SerialDevice::commandSent, this, [this](const QString &line) {
        appendLog(QStringLiteral("TX"), line);
    });
    connect(&m_serial, &SerialDevice::serialError, this, [this](const QString &message) {
        appendLog(QStringLiteral("ERR"), message);
    });
}

void GatewayController::setLogModel(AppLogModel *logModel) { m_logModel = logModel; }

QStringList GatewayController::portNames() const { return m_serial.portNames(); }
bool GatewayController::connected() const { return m_serial.connected(); }
QString GatewayController::statusText() const { return m_serial.statusText(); }
int GatewayController::can1Rx() const { return m_can1Rx; }
int GatewayController::can1Tx() const { return m_can1Tx; }
int GatewayController::can2Rx() const { return m_can2Rx; }
int GatewayController::can2Tx() const { return m_can2Tx; }
int GatewayController::busy() const { return m_busy; }
int GatewayController::errors() const { return m_errors; }
bool GatewayController::warning() const { return m_warning; }
int GatewayController::rpm() const { return m_rpm; }
int GatewayController::speed() const { return m_speed; }
int GatewayController::coolant() const { return m_coolant; }
bool GatewayController::ignition() const { return m_ignition; }
int GatewayController::alive() const { return m_alive; }
QString GatewayController::lastEngineRx() const { return m_lastEngineRx; }
bool GatewayController::doorFl() const { return m_doorFl; }
bool GatewayController::doorFr() const { return m_doorFr; }
bool GatewayController::doorRl() const { return m_doorRl; }
bool GatewayController::doorRr() const { return m_doorRr; }
bool GatewayController::turnLeft() const { return m_turnLeft; }
bool GatewayController::turnRight() const { return m_turnRight; }
bool GatewayController::highBeam() const { return m_highBeam; }
bool GatewayController::fogLamp() const { return m_fogLamp; }
QString GatewayController::lastBodyRx() const { return m_lastBodyRx; }
bool GatewayController::clusterRpmActive() const { return m_clusterRpmActive; }
bool GatewayController::clusterSpeedActive() const { return m_clusterSpeedActive; }
bool GatewayController::clusterBodyActive() const { return m_clusterBodyActive; }
QString GatewayController::latestFrameId() const { return m_latestFrameId; }
QString GatewayController::latestFrameBus() const { return m_latestFrameBus; }
QString GatewayController::latestFrameDir() const { return m_latestFrameDir; }
QString GatewayController::latestFrameDlc() const { return m_latestFrameDlc; }
QString GatewayController::latestFrameRaw() const { return m_latestFrameRaw; }
QStringList GatewayController::latestFrameBytes() const { return m_latestFrameBytes; }
QString GatewayController::latestFrameDecoded() const { return m_latestFrameDecoded; }

void GatewayController::refreshPorts() { m_serial.refreshPorts(); }

void GatewayController::resetMonitor()
{
    if (m_serial.connected()) {
        m_serial.disconnectPort();
    }

    m_can1Rx = 0;
    m_can1Tx = 0;
    m_can2Rx = 0;
    m_can2Tx = 0;
    m_busy = 0;
    m_errors = 0;
    m_warning = false;
    m_rpm = 0;
    m_speed = 0;
    m_coolant = 0;
    m_ignition = false;
    m_alive = 0;
    m_lastEngineRx = QStringLiteral("-");
    m_doorFl = false;
    m_doorFr = false;
    m_doorRl = false;
    m_doorRr = false;
    m_turnLeft = false;
    m_turnRight = false;
    m_highBeam = false;
    m_fogLamp = false;
    m_lastBodyRx = QStringLiteral("-");
    m_clusterRpmActive = false;
    m_clusterSpeedActive = false;
    m_clusterBodyActive = false;
    m_latestFrameId = QStringLiteral("-");
    m_latestFrameBus = QStringLiteral("-");
    m_latestFrameDir = QStringLiteral("-");
    m_latestFrameDlc = QStringLiteral("-");
    m_latestFrameRaw = QStringLiteral("-- -- -- -- -- -- -- --");
    m_latestFrameBytes = {"--", "--", "--", "--", "--", "--", "--", "--"};
    m_latestFrameDecoded = QStringLiteral("-");
    m_serial.refreshPorts();
    appendLog(QStringLiteral("SYS"), QStringLiteral("Gateway monitor reset"));
    emit dataChanged();
}

void GatewayController::connectToPort(const QString &portName, int baudRate)
{
    m_serial.connectToPort(portName, baudRate);
}

void GatewayController::disconnectPort()
{
    m_serial.disconnectPort();
}

void GatewayController::sendCommand(const QString &command)
{
    m_serial.sendLine(command);
}

void GatewayController::processLine(const QString &line)
{
    if (line.startsWith(QStringLiteral("[GW]"))) {
        parseGatewayStatus(line);
        appendLog(QStringLiteral("GW"), line);
        return;
    }

    if (line.startsWith(QStringLiteral("[RX")) || line.startsWith(QStringLiteral("[TX"))) {
        parseFrameLine(line);
        return;
    }

    appendLog(QStringLiteral("RX"), line);
}

void GatewayController::parseGatewayStatus(const QString &line)
{
    static const QRegularExpression regex(
        R"(\[GW\]\s+RX1=(\d+)\s+TX1=(\d+)\s+RX2=(\d+)\s+TX2=(\d+)\s+busy=(\d+)\s+err=(\d+)\s+warn=(\d+)(?:\s+rpm=(\d+)\s+speed=(\d+)\s+coolant=(\d+)\s+ign=(\d+)\s+alive=(\d+)\s+active=(\d+)\s+age=(\d+))?)");

    const QRegularExpressionMatch match = regex.match(line);
    if (!match.hasMatch()) {
        return;
    }

    m_can1Rx = match.captured(1).toInt();
    m_can1Tx = match.captured(2).toInt();
    m_can2Rx = match.captured(3).toInt();
    m_can2Tx = match.captured(4).toInt();
    m_busy = match.captured(5).toInt();
    m_errors = match.captured(6).toInt();
    m_warning = match.captured(7).toInt() != 0;

    if (!match.captured(8).isEmpty()) {
        m_rpm = match.captured(8).toInt();
        m_speed = match.captured(9).toInt();
        m_coolant = match.captured(10).toInt();
        m_ignition = match.captured(11).toInt() != 0;
        m_alive = match.captured(12).toInt();
        m_lastEngineRx = match.captured(14) + QStringLiteral(" ms");
    }

    emit dataChanged();
}

void GatewayController::parseFrameLine(const QString &line)
{
    static const QRegularExpression regex(
        R"(\[(RX|TX)([12])\]\s+id=0x([0-9A-Fa-f]+)\s+dlc=(\d+)\s+data=([0-9A-Fa-f ]+)(?:\s+st=(-?\d+))?)");

    const QRegularExpressionMatch match = regex.match(line);
    if (!match.hasMatch()) {
        appendLog(QStringLiteral("RX"), line);
        return;
    }

    const QString dir = match.captured(1).toUpper();
    const QString bus = QStringLiteral("CAN") + match.captured(2);
    const QString id = QStringLiteral("0x") + match.captured(3).toUpper();
    const QString dlc = match.captured(4);
    const QString payload = match.captured(5).simplified().toUpper();

    QList<int> bytes;
    QStringList normalizedBytes;
    const QStringList byteTexts = payload.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    for (const QString &byteText : byteTexts) {
        bool ok = false;
        const int value = byteText.toInt(&ok, 16);
        if (ok) {
            bytes.append(value & 0xFF);
            normalizedBytes.append(QStringLiteral("%1").arg(value & 0xFF, 2, 16, QLatin1Char('0')).toUpper());
        }
    }

    while (normalizedBytes.size() < 8) {
        normalizedBytes.append(QStringLiteral("--"));
    }

    const QString decoded = decodeFrame(id, bytes, dir);
    m_latestFrameId = id;
    m_latestFrameBus = bus;
    m_latestFrameDir = dir;
    m_latestFrameDlc = dlc;
    m_latestFrameRaw = payload;
    m_latestFrameBytes = normalizedBytes.mid(0, 8);
    m_latestFrameDecoded = decoded.isEmpty() ? QStringLiteral("No decoder registered for this CAN ID") : decoded;
    emit dataChanged();

    appendLog(dir + bus.right(1), QStringLiteral("%1 %2 dlc=%3 data=%4").arg(bus, id, dlc, payload), m_latestFrameDecoded);
}

QString GatewayController::decodeFrame(const QString &id, const QList<int> &bytes, const QString &dir)
{
    if (id == QStringLiteral("0x100") && bytes.size() >= 6) {
        m_rpm = bytes[0] | (bytes[1] << 8);
        m_speed = bytes[2] | (bytes[3] << 8);
        m_coolant = bytes[4];
        m_ignition = (bytes[5] & 0x01) != 0;
        m_alive = (bytes[5] & 0xFE) >> 1;
        m_lastEngineRx = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz"));
        return QStringLiteral("rpm=%1 speed=%2 coolant=%3 ign=%4 alive=%5")
            .arg(m_rpm)
            .arg(m_speed)
            .arg(m_coolant)
            .arg(m_ignition ? 1 : 0)
            .arg(m_alive);
    }

    if (id == QStringLiteral("0x390") && bytes.size() >= 8) {
        const bool anyDoor = bitAt(bytes, 4) || bitAt(bytes, 16);
        m_doorFl = anyDoor;
        m_doorFr = anyDoor;
        m_doorRl = anyDoor;
        m_doorRr = anyDoor;
        m_turnLeft = bitAt(bytes, 34);
        m_turnRight = bitAt(bytes, 35);
        m_highBeam = bitAt(bytes, 37) || bitAt(bytes, 49);
        m_fogLamp = bitAt(bytes, 58);
        m_lastBodyRx = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz"));
        if (dir == QStringLiteral("TX")) {
            m_clusterBodyActive = true;
        }
        return QStringLiteral("door=%1 left=%2 right=%3 high=%4 fog=%5")
            .arg(anyDoor ? 1 : 0)
            .arg(m_turnLeft ? 1 : 0)
            .arg(m_turnRight ? 1 : 0)
            .arg(m_highBeam ? 1 : 0)
            .arg(m_fogLamp ? 1 : 0);
    }

    if (id == QStringLiteral("0x280")) {
        QString decoded = QStringLiteral("cluster rpm frame");
        if (bytes.size() >= 5) {
            const int clusterRpm = static_cast<int>(leSignal(bytes, 16, 16) / 4U);
            decoded = QStringLiteral("cluster_rpm=%1 rpm").arg(clusterRpm);
        }
        if (dir == QStringLiteral("TX")) {
            m_clusterRpmActive = true;
        }
        return decoded;
    }

    if (id == QStringLiteral("0x1A0")) {
        QString decoded = QStringLiteral("cluster speed frame");
        if (bytes.size() >= 4) {
            const int clusterSpeed = (bytes[2] | (bytes[3] << 8)) / 80;
            decoded = QStringLiteral("cluster_speed=%1 km/h").arg(clusterSpeed);
        }
        if (dir == QStringLiteral("TX")) {
            m_clusterSpeedActive = true;
        }
        return decoded;
    }

    if (id == QStringLiteral("0x288")) {
        if (bytes.size() >= 2) {
            return QStringLiteral("cluster_coolant=%1 C").arg(bytes[1]);
        }
        return QStringLiteral("cluster coolant frame");
    }

    if (id == QStringLiteral("0x5A0")) {
        if (bytes.size() >= 3) {
            const int needleSpeed = bytes[2] * 2;
            return QStringLiteral("cluster_speed_needle=%1 km/h").arg(needleSpeed);
        }
        return QStringLiteral("cluster speed needle frame");
    }

    if (id == QStringLiteral("0x481") || id == QStringLiteral("0x480")) {
        const bool rpmWarning = !bytes.isEmpty() && ((bytes[0] & 0x01) != 0);
        const bool overheat = !bytes.isEmpty() && ((bytes[0] & 0x02) != 0);
        const bool general = !bytes.isEmpty() && ((bytes[0] & 0x04) != 0);
        const int coolant = bytes.size() >= 2 ? bytes[1] : 0;
        const int rpm = bytes.size() >= 4 ? (bytes[2] | (bytes[3] << 8)) : 0;
        m_warning = rpmWarning || overheat || general;
        return QStringLiteral("engine_warning rpm_warn=%1 coolant_warn=%2 general=%3 rpm=%4 coolant=%5")
            .arg(rpmWarning ? 1 : 0)
            .arg(overheat ? 1 : 0)
            .arg(general ? 1 : 0)
            .arg(rpm)
            .arg(coolant);
    }

    if ((id == QStringLiteral("0x714") || id == QStringLiteral("0x7E0") ||
         id == QStringLiteral("0x77E") || id == QStringLiteral("0x7E8")) && !bytes.isEmpty()) {
        return QStringLiteral("UDS/ISO-TP single frame pci=0x%1 sid=0x%2")
            .arg(bytes.value(0), 2, 16, QLatin1Char('0')).toUpper()
            .arg(bytes.value(1), 2, 16, QLatin1Char('0')).toUpper();
    }

    return {};
}

quint32 GatewayController::leSignal(const QList<int> &bytes, int startBit, int bitLength) const
{
    quint32 raw = 0;
    for (int i = 0; i < bitLength; ++i) {
        const int bitPos = startBit + i;
        const int byteIndex = bitPos / 8;
        const int bitIndex = bitPos % 8;
        if (byteIndex >= 0 && byteIndex < bytes.size() && ((bytes[byteIndex] & (1 << bitIndex)) != 0)) {
            raw |= (1u << i);
        }
    }
    return raw;
}

bool GatewayController::bitAt(const QList<int> &bytes, int bit) const
{
    const int byteIndex = bit / 8;
    const int bitIndex = bit % 8;
    if (byteIndex < 0 || byteIndex >= bytes.size()) {
        return false;
    }
    return (bytes[byteIndex] & (1 << bitIndex)) != 0;
}

void GatewayController::appendLog(const QString &direction, const QString &text, const QString &decoded)
{
    if (m_logModel) {
        m_logModel->append(QStringLiteral("Gateway"), direction, text, decoded);
    }
}
