#include "SerialBridge.h"

#include <QDateTime>
#include <QRegularExpression>
#include <QSerialPortInfo>
#include <algorithm>

SerialBridge::SerialBridge(QObject *parent)
    : QObject(parent)
{
    connect(&m_serial, &QSerialPort::readyRead, this, &SerialBridge::onReadyRead);
    connect(&m_serial, &QSerialPort::errorOccurred, this, &SerialBridge::onErrorOccurred);
    connect(&m_pollTimer, &QTimer::timeout, this, &SerialBridge::onPollSerial);
    m_pollTimer.setInterval(25);
    refreshPorts();
    setStatusText("Select a serial port");
}

QStringList SerialBridge::portNames() const { return m_portNames; }
bool SerialBridge::connected() const { return m_serial.isOpen(); }
QString SerialBridge::statusText() const { return m_statusText; }

int SerialBridge::can1Rx() const { return m_can1Rx; }
int SerialBridge::can1Tx() const { return m_can1Tx; }
int SerialBridge::can2Rx() const { return m_can2Rx; }
int SerialBridge::can2Tx() const { return m_can2Tx; }
int SerialBridge::busy() const { return m_busy; }
int SerialBridge::errors() const { return m_errors; }
bool SerialBridge::warning() const { return m_warning; }
int SerialBridge::routeMatched() const { return m_routeMatched; }
int SerialBridge::routeOk() const { return m_routeOk; }
int SerialBridge::routeFail() const { return m_routeFail; }
int SerialBridge::routeIgnored() const { return m_routeIgnored; }

int SerialBridge::rpm() const { return m_rpm; }
int SerialBridge::speed() const { return m_speed; }
int SerialBridge::coolant() const { return m_coolant; }
bool SerialBridge::ignition() const { return m_ignition; }
int SerialBridge::alive() const { return m_alive; }
QString SerialBridge::lastEngineRx() const { return m_lastEngineRx; }

bool SerialBridge::doorFl() const { return m_doorFl; }
bool SerialBridge::doorFr() const { return m_doorFr; }
bool SerialBridge::doorRl() const { return m_doorRl; }
bool SerialBridge::doorRr() const { return m_doorRr; }
bool SerialBridge::turnLeft() const { return m_turnLeft; }
bool SerialBridge::turnRight() const { return m_turnRight; }
bool SerialBridge::highBeam() const { return m_highBeam; }
bool SerialBridge::fogLamp() const { return m_fogLamp; }
QString SerialBridge::lastBodyRx() const { return m_lastBodyRx; }

bool SerialBridge::clusterRpmActive() const { return m_clusterRpmActive; }
bool SerialBridge::clusterSpeedActive() const { return m_clusterSpeedActive; }
bool SerialBridge::clusterSpeedNeedleActive() const { return m_clusterSpeedNeedleActive; }
bool SerialBridge::clusterCoolantActive() const { return m_clusterCoolantActive; }
bool SerialBridge::clusterIgnActive() const { return m_clusterIgnActive; }
bool SerialBridge::clusterBodyActive() const { return m_clusterBodyActive; }
bool SerialBridge::clusterTurnActive() const { return m_clusterTurnActive; }
bool SerialBridge::adasValid() const { return m_adasValid; }
int SerialBridge::adasRisk() const { return m_adasRisk; }
int SerialBridge::adasFault() const { return m_adasFault; }
int SerialBridge::adasDtc() const { return m_adasDtc; }
int SerialBridge::adasFront() const { return m_adasFront; }
int SerialBridge::adasRear() const { return m_adasRear; }
int SerialBridge::adasSpeed() const { return m_adasSpeed; }
int SerialBridge::adasAlive() const { return m_adasAlive; }
QString SerialBridge::latestFrameId() const { return m_latestFrameId; }
QString SerialBridge::latestFrameBus() const { return m_latestFrameBus; }
QString SerialBridge::latestFrameDir() const { return m_latestFrameDir; }
QString SerialBridge::latestFrameDlc() const { return m_latestFrameDlc; }
QString SerialBridge::latestFrameRaw() const { return m_latestFrameRaw; }
QStringList SerialBridge::latestFrameBytes() const { return m_latestFrameBytes; }
QString SerialBridge::latestFrameDecoded() const { return m_latestFrameDecoded; }

void SerialBridge::refreshPorts()
{
    QStringList names;
    const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &port : ports) {
        const QString location = port.systemLocation();
        if (!location.startsWith("/dev/cu.")) {
            continue;
        }

        const QString lowerName = port.portName().toLower();
        const QString lowerLocation = location.toLower();
        if (lowerName.contains("bluetooth") ||
            lowerName.contains("debug-console") ||
            lowerName.contains("wlan") ||
            lowerLocation.contains("bluetooth") ||
            lowerLocation.contains("debug-console") ||
            lowerLocation.contains("wlan")) {
            continue;
        }

        names.append(location);
    }

    std::sort(names.begin(), names.end(), [](const QString &a, const QString &b) {
        const bool aUsb = a.contains("usb", Qt::CaseInsensitive);
        const bool bUsb = b.contains("usb", Qt::CaseInsensitive);
        if (aUsb != bUsb) {
            return aUsb;
        }
        return a < b;
    });

    if (names != m_portNames) {
        m_portNames = names;
        emit portNamesChanged();
    }

    if (m_portNames.isEmpty()) {
        setStatusText("No serial ports found");
    }
}

void SerialBridge::resetMonitor()
{
    if (m_serial.isOpen()) {
        m_pollTimer.stop();
        m_serial.close();
        emit connectionChanged();
    }

    m_rxBuffer.clear();
    m_can1Rx = 0;
    m_can1Tx = 0;
    m_can2Rx = 0;
    m_can2Tx = 0;
    m_busy = 0;
    m_errors = 0;
    m_warning = false;
    m_routeMatched = 0;
    m_routeOk = 0;
    m_routeFail = 0;
    m_routeIgnored = 0;
    m_rpm = 0;
    m_speed = 0;
    m_coolant = 0;
    m_ignition = false;
    m_alive = 0;
    m_lastEngineRx = "-";
    m_doorFl = false;
    m_doorFr = false;
    m_doorRl = false;
    m_doorRr = false;
    m_turnLeft = false;
    m_turnRight = false;
    m_highBeam = false;
    m_fogLamp = false;
    m_lastBodyRx = "-";
    m_clusterRpmActive = false;
    m_clusterSpeedActive = false;
    m_clusterSpeedNeedleActive = false;
    m_clusterCoolantActive = false;
    m_clusterIgnActive = false;
    m_clusterBodyActive = false;
    m_clusterTurnActive = false;
    m_adasValid = false;
    m_adasRisk = 0;
    m_adasFault = 0;
    m_adasDtc = 0;
    m_adasFront = 0;
    m_adasRear = 0;
    m_adasSpeed = 0;
    m_adasAlive = 0;
    m_latestFrameId = "-";
    m_latestFrameBus = "-";
    m_latestFrameDir = "-";
    m_latestFrameDlc = "-";
    m_latestFrameRaw = "-- -- -- -- -- -- -- --";
    m_latestFrameBytes = {"--", "--", "--", "--", "--", "--", "--", "--"};
    m_latestFrameDecoded = "-";

    refreshPorts();
    setStatusText("Serial monitor reset");
    emit dataChanged();
}

void SerialBridge::connectToPort(const QString &portName, int baudRate)
{
    if (m_serial.isOpen()) {
        m_pollTimer.stop();
        m_serial.close();
        emit connectionChanged();
    }

    if (portName.isEmpty()) {
        setStatusText("Select a serial port first");
        return;
    }

    m_serial.setPortName(portName);
    m_serial.setBaudRate(baudRate);
    m_serial.setDataBits(QSerialPort::Data8);
    m_serial.setParity(QSerialPort::NoParity);
    m_serial.setStopBits(QSerialPort::OneStop);
    m_serial.setFlowControl(QSerialPort::NoFlowControl);

    if (!m_serial.open(QIODevice::ReadWrite)) {
        setStatusText("Open failed: " + m_serial.errorString());
        emit connectionChanged();
        return;
    }

    m_serial.setDataTerminalReady(true);
    m_serial.setRequestToSend(true);
    m_serial.clear(QSerialPort::AllDirections);
    m_rxBuffer.clear();
    m_pollTimer.start();
    setStatusText("Connected " + portName + " @ " + QString::number(baudRate));
    emit connectionChanged();

    sendCommand("log on");
    sendCommand("canlog rate 100");
    sendCommand("canlog off");
}

void SerialBridge::disconnectPort()
{
    m_pollTimer.stop();
    if (m_serial.isOpen()) {
        m_serial.setRequestToSend(false);
        m_serial.setDataTerminalReady(false);
        m_serial.close();
    }
    setStatusText("Disconnected");
    emit connectionChanged();
}

void SerialBridge::sendCommand(const QString &command)
{
    if (!m_serial.isOpen()) {
        setStatusText("Serial port is not connected");
        return;
    }

    QByteArray payload = command.toUtf8();
    if (!payload.endsWith('\n')) {
        payload.append("\r\n");
    }
    m_serial.write(payload);
    m_serial.flush();
}

void SerialBridge::onReadyRead()
{
    m_rxBuffer.append(m_serial.readAll());

    while (true) {
        const int newline = m_rxBuffer.indexOf('\n');
        if (newline < 0) {
            break;
        }

        QByteArray lineBytes = m_rxBuffer.left(newline);
        m_rxBuffer.remove(0, newline + 1);
        lineBytes.replace("\r", "");

        const QString line = QString::fromUtf8(lineBytes).trimmed();
        if (!line.isEmpty()) {
            processLine(line);
        }
    }
}

void SerialBridge::onPollSerial()
{
    if (m_serial.isOpen() && m_serial.bytesAvailable() > 0) {
        onReadyRead();
    }
}

void SerialBridge::onErrorOccurred(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) {
        return;
    }

    if (error == QSerialPort::ResourceError) {
        setStatusText("Serial disconnected: " + m_serial.errorString());
        m_pollTimer.stop();
        m_serial.close();
        emit connectionChanged();
    }
}

void SerialBridge::setStatusText(const QString &text)
{
    if (m_statusText == text) {
        return;
    }
    m_statusText = text;
    emit statusTextChanged();
}

void SerialBridge::processLine(const QString &line)
{
    const int gwIndex = line.indexOf("[GW]");
    if (gwIndex >= 0) {
        QString gatewayLine = line.mid(gwIndex);
        const int nextGwIndex = gatewayLine.indexOf("[GW]", 4);
        if (nextGwIndex > 0) {
            gatewayLine = gatewayLine.left(nextGwIndex).trimmed();
        }
        parseGatewayStatus(gatewayLine);
        emit gatewayLineReceived(nowString(), gatewayLine);
        return;
    }

    const int rxIndex = line.indexOf("[RX");
    const int txIndex = line.indexOf("[TX");
    int frameIndex = -1;
    if (rxIndex >= 0 && txIndex >= 0) {
        frameIndex = qMin(rxIndex, txIndex);
    } else if (rxIndex >= 0) {
        frameIndex = rxIndex;
    } else if (txIndex >= 0) {
        frameIndex = txIndex;
    }

    if (frameIndex >= 0) {
        parseFrameLine(line.mid(frameIndex));
        return;
    }

    emit rawLineReceived(nowString(), line);
}

void SerialBridge::parseGatewayStatus(const QString &line)
{
    bool matched = false;
    auto valueFor = [&](const QString &key, int fallback) {
        const QRegularExpression regex("\\b" + QRegularExpression::escape(key) + "=(-?(?:0x)?[0-9A-Fa-f]+)");
        const QRegularExpressionMatch match = regex.match(line);
        if (!match.hasMatch()) {
            return fallback;
        }

        bool ok = false;
        QString text = match.captured(1);
        const bool negative = text.startsWith('-');
        if (negative) {
            text.remove(0, 1);
        }
        const int base = text.startsWith("0x", Qt::CaseInsensitive) ? 16 : 10;
        if (base == 16) {
            text.remove(0, 2);
        }
        int value = text.toInt(&ok, base);
        if (negative) {
            value = -value;
        }
        if (ok) {
            matched = true;
            return value;
        }
        return fallback;
    };

    m_can1Rx = valueFor("RX1", m_can1Rx);
    m_can1Tx = valueFor("TX1", m_can1Tx);
    m_can2Rx = valueFor("RX2", m_can2Rx);
    m_can2Tx = valueFor("TX2", m_can2Tx);
    m_busy = valueFor("busy", m_busy);
    m_errors = valueFor("err", m_errors);

    m_routeMatched = valueFor("route", m_routeMatched);
    m_routeOk = valueFor("ok", m_routeOk);
    m_routeFail = valueFor("fail", m_routeFail);
    m_routeIgnored = valueFor("ignore", m_routeIgnored);

    m_adasValid = valueFor("adas", m_adasValid ? 1 : 0) != 0;
    m_adasRisk = valueFor("risk", m_adasRisk);
    m_adasFault = valueFor("fault", m_adasFault);
    m_adasDtc = valueFor("dtc", m_adasDtc);
    m_adasFront = valueFor("front", m_adasFront);
    m_adasRear = valueFor("rear", m_adasRear);
    m_adasSpeed = valueFor("speed", m_adasSpeed);
    m_adasAlive = valueFor("alive", m_adasAlive);

    const int engineValid = valueFor("eng", -1);
    if (engineValid >= 0) {
        m_rpm = valueFor("rpm", m_rpm);
        m_speed = valueFor("spd1", m_speed);
        m_coolant = valueFor("coolant", m_coolant);
        m_ignition = valueFor("ign", m_ignition ? 1 : 0) != 0;
        m_lastEngineRx = nowString();
    }

    const int bodyValid = valueFor("body", -1);
    if (bodyValid >= 0) {
        m_turnLeft = valueFor("left", m_turnLeft ? 1 : 0) != 0;
        m_turnRight = valueFor("right", m_turnRight ? 1 : 0) != 0;
        m_lastBodyRx = nowString();
    }

    m_warning = valueFor("warn", (m_adasRisk >= 2 || m_adasFault != 0) ? 1 : 0) != 0 ||
                m_adasRisk >= 2 ||
                m_adasFault != 0;

    if (!matched) {
        return;
    }

    emit dataChanged();
}

void SerialBridge::parseFrameLine(const QString &line)
{
    static const QRegularExpression regex(
        R"(\[(RX|TX)([12])\]\s+id=0x([0-9A-Fa-f]+)\s+dlc=(\d+)\s+data=([0-9A-Fa-f ]+)(?:\s+st=(-?\d+))?)");
    const QRegularExpressionMatch match = regex.match(line);
    if (!match.hasMatch()) {
        emit rawLineReceived(nowString(), line);
        return;
    }

    const QString dir = match.captured(1);
    const QString bus = "CAN" + match.captured(2);
    const QString id = "0x" + match.captured(3).toUpper();
    const QString dlc = match.captured(4);
    const QString payload = match.captured(5).simplified().toUpper();

    QList<int> bytes;
    QStringList normalizedBytes;
    const QStringList byteTexts = payload.split(' ', Qt::SkipEmptyParts);
    for (const QString &byteText : byteTexts) {
        bool ok = false;
        const int value = byteText.toInt(&ok, 16);
        if (ok) {
            bytes.append(value & 0xFF);
            normalizedBytes.append(QString("%1").arg(value & 0xFF, 2, 16, QLatin1Char('0')).toUpper());
        }
    }

    while (normalizedBytes.size() < 8) {
        normalizedBytes.append("--");
    }

    const QString decoded = decodeFrame(id, bytes, dir);
    m_latestFrameId = id;
    m_latestFrameBus = bus;
    m_latestFrameDir = dir;
    m_latestFrameDlc = dlc;
    m_latestFrameRaw = payload;
    m_latestFrameBytes = normalizedBytes.mid(0, 8);
    m_latestFrameDecoded = decoded.isEmpty() ? "No decoder registered for this CAN ID" : decoded;
    emit dataChanged();

    emit frameLineReceived(nowString(), bus, dir, id, dlc, payload, decoded);
}

QString SerialBridge::decodeFrame(const QString &id, const QList<int> &bytes, const QString &dir)
{
    if (id == "0x100" && bytes.size() >= 6) {
        m_ignition = (bytes[5] & 0x01) != 0;
        m_lastEngineRx = nowString();
        if (dir == "TX") {
            m_clusterIgnActive = true;
        }
        emit dataChanged();
        return QString("board_a_ign=%1").arg(m_ignition ? 1 : 0);
    }

    if (id == "0x531" && bytes.size() >= 3) {
        m_turnLeft = (bytes[2] & 0x01) != 0;
        m_turnRight = (bytes[2] & 0x02) != 0;
        const bool hazard = (bytes[2] & 0x04) != 0;
        m_lastBodyRx = nowString();
        if (dir == "TX") {
            m_clusterTurnActive = true;
        }
        emit dataChanged();
        return QString("board_d_turn left=%1 right=%2 hazard=%3")
            .arg(m_turnLeft ? 1 : 0)
            .arg(m_turnRight ? 1 : 0)
            .arg(hazard ? 1 : 0);
    }

    if (id == "0x3A0" && bytes.size() >= 8) {
        m_adasValid = true;
        m_adasRisk = bytes[1];
        m_adasFront = bytes[2];
        m_adasRear = bytes[3];
        m_adasFault = bytes[4];
        m_adasSpeed = bytes[5];
        m_adasAlive = bytes[7];
        m_warning = m_adasRisk >= 2 || m_adasFault != 0;
        emit dataChanged();
        return QString("board_e_adas flags=0x%1 risk=%2 front=%3 rear=%4 speed=%5 fault=0x%6 alive=%7")
            .arg(bytes[0], 2, 16, QLatin1Char('0')).toUpper()
            .arg(m_adasRisk)
            .arg(m_adasFront)
            .arg(m_adasRear)
            .arg(m_adasSpeed)
            .arg(m_adasFault, 2, 16, QLatin1Char('0')).toUpper()
            .arg(m_adasAlive);
    }

    if (id == "0x390" && bytes.size() >= 8) {
        const bool anyDoor = bitAt(bytes, 4) || bitAt(bytes, 16);
        m_doorFl = anyDoor;
        m_doorFr = anyDoor;
        m_doorRl = anyDoor;
        m_doorRr = anyDoor;
        m_turnLeft = bitAt(bytes, 34);
        m_turnRight = bitAt(bytes, 35);
        m_highBeam = bitAt(bytes, 37) || bitAt(bytes, 49);
        m_fogLamp = bitAt(bytes, 58);
        m_lastBodyRx = nowString();
        if (dir == "TX") {
            m_clusterBodyActive = true;
        }
        emit dataChanged();
        return QString("door=%1 left=%2 right=%3 high=%4 fog=%5")
            .arg(anyDoor ? 1 : 0)
            .arg(m_turnLeft ? 1 : 0)
            .arg(m_turnRight ? 1 : 0)
            .arg(m_highBeam ? 1 : 0)
            .arg(m_fogLamp ? 1 : 0);
    }

    if (id == "0x280") {
        QString decoded = "cluster rpm frame";
        if (bytes.size() >= 5) {
            const int clusterRpm = static_cast<int>(leSignal(bytes, 16, 16) / 4U);
            m_rpm = clusterRpm;
            m_lastEngineRx = nowString();
            decoded = QString("cluster_rpm=%1").arg(clusterRpm);
        }

        if (dir == "TX") {
            m_clusterRpmActive = true;
            emit dataChanged();
        }
        return decoded;
    }

    if (id == "0x1A0") {
        QString decoded = "cluster speed frame";
        if (bytes.size() >= 4) {
            const int clusterSpeed = ((bytes[2] | (bytes[3] << 8)) / 80);
            m_speed = clusterSpeed;
            m_lastEngineRx = nowString();
            decoded = QString("cluster_speed=%1 km/h").arg(clusterSpeed);
        }

        if (dir == "TX") {
            m_clusterSpeedActive = true;
            emit dataChanged();
        }
        return decoded;
    }

    if (id == "0x288") {
        if (bytes.size() >= 2) {
            m_coolant = bytes[1];
            m_lastEngineRx = nowString();
            if (dir == "TX") {
                m_clusterCoolantActive = true;
            }
            emit dataChanged();
            return QString("cluster_coolant_needle=%1").arg(m_coolant);
        }
        return "cluster coolant frame";
    }

    if (id == "0x5A0") {
        if (bytes.size() >= 3) {
            m_speed = bytes[2];
            m_lastEngineRx = nowString();
            if (dir == "TX") {
                m_clusterSpeedNeedleActive = true;
            }
            emit dataChanged();
            return QString("cluster_speed_needle=%1 km/h").arg(m_speed);
        }
        return "cluster speed needle frame";
    }

    if (id == "0x480") {
        const bool heat = bitAt(bytes, 12);
        const bool lamp = bitAt(bytes, 40);
        const bool faultLamp = bitAt(bytes, 44);
        const bool text = bitAt(bytes, 54);
        m_warning = heat || lamp || faultLamp || text;
        emit dataChanged();
        return QString("mMotor_5 warn heat=%1 lamp=%2 fault=%3 text=%4")
            .arg(heat ? 1 : 0)
            .arg(lamp ? 1 : 0)
            .arg(faultLamp ? 1 : 0)
            .arg(text ? 1 : 0);
    }

    return "";
}

uint32_t SerialBridge::leSignal(const QList<int> &bytes, int startBit, int bitLength) const
{
    uint32_t raw = 0;
    for (int i = 0; i < bitLength; ++i) {
        const int bitPos = startBit + i;
        const int byteIndex = bitPos / 8;
        const int bitIndex = bitPos % 8;
        if (byteIndex >= 0 && byteIndex < bytes.size() &&
            ((bytes[byteIndex] & (1 << bitIndex)) != 0)) {
            raw |= (1u << i);
        }
    }
    return raw;
}

QString SerialBridge::nowString() const
{
    return QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
}

bool SerialBridge::bitAt(const QList<int> &bytes, int bit) const
{
    const int byteIndex = bit / 8;
    const int bitIndex = bit % 8;
    if (byteIndex < 0 || byteIndex >= bytes.size()) {
        return false;
    }
    return (bytes[byteIndex] & (1 << bitIndex)) != 0;
}
