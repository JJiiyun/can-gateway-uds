#include "BodyController.h"

#include <QHash>
#include <QRegularExpression>

#include "models/AppLogModel.h"

BodyController::BodyController(QObject *parent) : QObject(parent)
{
    connect(&m_serial, &SerialDevice::portNamesChanged, this, &BodyController::portNamesChanged);
    connect(&m_serial, &SerialDevice::connectedChanged, this, &BodyController::connectedChanged);
    connect(&m_serial, &SerialDevice::statusTextChanged, this, &BodyController::statusTextChanged);
    connect(&m_serial, &SerialDevice::lineReceived, this, &BodyController::processLine);
    connect(&m_serial, &SerialDevice::commandSent, this, [this](const QString &line) {
        appendLog(QStringLiteral("TX"), line);
    });
    connect(&m_serial, &SerialDevice::serialError, this, [this](const QString &message) {
        appendLog(QStringLiteral("ERR"), message);
    });
}

void BodyController::setLogModel(AppLogModel *logModel) { m_logModel = logModel; }

QStringList BodyController::portNames() const { return m_serial.portNames(); }
bool BodyController::connected() const { return m_serial.connected(); }
QString BodyController::statusText() const { return m_serial.statusText(); }
QString BodyController::mode() const { return m_mode; }
bool BodyController::ignition() const { return m_ignition; }
bool BodyController::doorFl() const { return m_doorFl; }
bool BodyController::doorFr() const { return m_doorFr; }
bool BodyController::doorRl() const { return m_doorRl; }
bool BodyController::doorRr() const { return m_doorRr; }
bool BodyController::turnLeft() const { return m_turnLeft; }
bool BodyController::turnRight() const { return m_turnRight; }
bool BodyController::highBeam() const { return m_highBeam; }
bool BodyController::fogLamp() const { return m_fogLamp; }
int BodyController::txCount() const { return m_txCount; }

void BodyController::refreshPorts() { m_serial.refreshPorts(); }

void BodyController::connectToPort(const QString &portName, int baudRate)
{
    m_serial.connectToPort(portName, baudRate);
    if (m_serial.connected()) {
        monitorOn(200);
        status();
    }
}

void BodyController::disconnectPort()
{
    if (m_serial.connected()) {
        m_serial.sendLine(QStringLiteral("body monitor off"));
    }
    m_serial.disconnectPort();
}

void BodyController::setMode(const QString &mode)
{
    const QString normalized = mode.trimmed().toLower();
    if (normalized != QStringLiteral("gpio") && normalized != QStringLiteral("uart")) {
        appendLog(QStringLiteral("ERR"), QStringLiteral("Unsupported body mode: %1").arg(mode));
        return;
    }
    m_mode = normalized;
    emit dataChanged();
    sendCommand(QStringLiteral("body mode %1").arg(normalized));
}

void BodyController::setIgnition(bool enabled)
{
    setBoolLocal(m_ignition, enabled);
    sendBooleanCommand(QStringLiteral("body ign"), enabled);
}

void BodyController::setDoorFl(bool enabled)
{
    setBoolLocal(m_doorFl, enabled);
    sendBooleanCommand(QStringLiteral("body door fl"), enabled);
}

void BodyController::setDoorFr(bool enabled)
{
    setBoolLocal(m_doorFr, enabled);
    sendBooleanCommand(QStringLiteral("body door fr"), enabled);
}

void BodyController::setDoorRl(bool enabled)
{
    setBoolLocal(m_doorRl, enabled);
    sendBooleanCommand(QStringLiteral("body door rl"), enabled);
}

void BodyController::setDoorRr(bool enabled)
{
    setBoolLocal(m_doorRr, enabled);
    sendBooleanCommand(QStringLiteral("body door rr"), enabled);
}

void BodyController::setTurnLeft(bool enabled)
{
    setBoolLocal(m_turnLeft, enabled);
    sendBooleanCommand(QStringLiteral("body turn left"), enabled);
}

void BodyController::setTurnRight(bool enabled)
{
    setBoolLocal(m_turnRight, enabled);
    sendBooleanCommand(QStringLiteral("body turn right"), enabled);
}

void BodyController::setHighBeam(bool enabled)
{
    setBoolLocal(m_highBeam, enabled);
    sendBooleanCommand(QStringLiteral("body high"), enabled);
}

void BodyController::setFogLamp(bool enabled)
{
    setBoolLocal(m_fogLamp, enabled);
    sendBooleanCommand(QStringLiteral("body fog"), enabled);
}

void BodyController::allDoors(bool open)
{
    m_doorFl = open;
    m_doorFr = open;
    m_doorRl = open;
    m_doorRr = open;
    emit dataChanged();
    sendCommand(QStringLiteral("body all doors %1").arg(open ? 1 : 0));
}

void BodyController::sendState()
{
    sendCommand(QStringLiteral("body mode uart"));
    sendCommand(QStringLiteral("body ign %1").arg(m_ignition ? 1 : 0));
    sendCommand(QStringLiteral("body door fl %1").arg(m_doorFl ? 1 : 0));
    sendCommand(QStringLiteral("body door fr %1").arg(m_doorFr ? 1 : 0));
    sendCommand(QStringLiteral("body door rl %1").arg(m_doorRl ? 1 : 0));
    sendCommand(QStringLiteral("body door rr %1").arg(m_doorRr ? 1 : 0));
    sendCommand(QStringLiteral("body turn left %1").arg(m_turnLeft ? 1 : 0));
    sendCommand(QStringLiteral("body turn right %1").arg(m_turnRight ? 1 : 0));
    sendCommand(QStringLiteral("body high %1").arg(m_highBeam ? 1 : 0));
    sendCommand(QStringLiteral("body fog %1").arg(m_fogLamp ? 1 : 0));
}

void BodyController::monitorOn(int intervalMs)
{
    sendCommand(QStringLiteral("body monitor on %1").arg(qMax(50, intervalMs)));
}

void BodyController::monitorOff()
{
    sendCommand(QStringLiteral("body monitor off"));
}

void BodyController::status()
{
    sendCommand(QStringLiteral("body status"));
}

void BodyController::sendCommand(const QString &command)
{
    m_serial.sendLine(command);
}

void BodyController::processLine(const QString &line)
{
    appendLog(QStringLiteral("RX"), line);
    if (line.contains(QStringLiteral("BODY ")) || line.startsWith(QStringLiteral("BODY"))) {
        parseBodyLine(line);
    }
}

void BodyController::parseBodyLine(const QString &line)
{
    QString payload = line;
    const int bodyIndex = payload.indexOf(QStringLiteral("BODY"));
    if (bodyIndex >= 0) {
        payload = payload.mid(bodyIndex + 4).trimmed();
    }

    static const QRegularExpression keyValueRegex(QStringLiteral("(\\w+)=([^\\s]+)"));
    QHash<QString, QString> values;
    QRegularExpressionMatchIterator it = keyValueRegex.globalMatch(payload);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        values.insert(match.captured(1).toLower(), match.captured(2));
    }

    bool changed = false;
    auto setBoolFromValue = [&values, &changed](const QString &key, bool &field) {
        if (!values.contains(key)) {
            return;
        }
        const bool value = values.value(key).toInt() != 0;
        if (field != value) {
            field = value;
            changed = true;
        }
    };

    if (values.contains(QStringLiteral("mode"))) {
        const QString value = values.value(QStringLiteral("mode")).toLower();
        if (m_mode != value) {
            m_mode = value;
            changed = true;
        }
    }

    setBoolFromValue(QStringLiteral("ign"), m_ignition);
    setBoolFromValue(QStringLiteral("fl"), m_doorFl);
    setBoolFromValue(QStringLiteral("fr"), m_doorFr);
    setBoolFromValue(QStringLiteral("rl"), m_doorRl);
    setBoolFromValue(QStringLiteral("rr"), m_doorRr);
    setBoolFromValue(QStringLiteral("left"), m_turnLeft);
    setBoolFromValue(QStringLiteral("right"), m_turnRight);
    setBoolFromValue(QStringLiteral("high"), m_highBeam);
    setBoolFromValue(QStringLiteral("fog"), m_fogLamp);

    if (values.contains(QStringLiteral("tx"))) {
        bool ok = false;
        const int value = values.value(QStringLiteral("tx")).toInt(&ok);
        if (ok && m_txCount != value) {
            m_txCount = value;
            changed = true;
        }
    }

    if (changed) {
        emit dataChanged();
    }
}

void BodyController::appendLog(const QString &direction, const QString &text)
{
    if (m_logModel) {
        m_logModel->append(QStringLiteral("Body"), direction, text);
    }
}

void BodyController::sendBooleanCommand(const QString &command, bool enabled)
{
    sendCommand(QStringLiteral("%1 %2").arg(command).arg(enabled ? 1 : 0));
}

void BodyController::setBoolLocal(bool &field, bool value)
{
    if (field == value) {
        return;
    }
    field = value;
    emit dataChanged();
}
