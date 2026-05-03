#include "EngineController.h"

#include <QHash>
#include <QRegularExpression>

#include "models/AppLogModel.h"

namespace {
int percentClamp(int value)
{
    return qBound(0, value, 100);
}

int valueToInt(const QHash<QString, QString> &values, const QString &key, bool *ok)
{
    return values.value(key).toInt(ok);
}
}

EngineController::EngineController(QObject *parent) : QObject(parent)
{
    m_pedalDebounce.setInterval(80);
    m_pedalDebounce.setSingleShot(true);

    connect(&m_pedalDebounce, &QTimer::timeout, this, &EngineController::applyPedal);
    connect(&m_serial, &SerialDevice::portNamesChanged, this, &EngineController::portNamesChanged);
    connect(&m_serial, &SerialDevice::connectedChanged, this, &EngineController::connectedChanged);
    connect(&m_serial, &SerialDevice::statusTextChanged, this, &EngineController::statusTextChanged);
    connect(&m_serial, &SerialDevice::lineReceived, this, &EngineController::processLine);
    connect(&m_serial, &SerialDevice::commandSent, this, [this](const QString &line) {
        appendLog(QStringLiteral("TX"), line);
    });
    connect(&m_serial, &SerialDevice::serialError, this, [this](const QString &message) {
        appendLog(QStringLiteral("ERR"), message);
    });
}

void EngineController::setLogModel(AppLogModel *logModel)
{
    m_logModel = logModel;
}

QStringList EngineController::portNames() const { return m_serial.portNames(); }
bool EngineController::connected() const { return m_serial.connected(); }
QString EngineController::statusText() const { return m_serial.statusText(); }
QString EngineController::mode() const { return m_mode; }
int EngineController::throttle() const { return m_throttle; }
int EngineController::brake() const { return m_brake; }
int EngineController::rpm() const { return m_rpm; }
int EngineController::speed() const { return m_speed; }
int EngineController::coolant() const { return m_coolant; }
int EngineController::txCount() const { return m_txCount; }
bool EngineController::liveUpdate() const { return m_liveUpdate; }

void EngineController::setLiveUpdate(bool enabled)
{
    if (m_liveUpdate == enabled) {
        return;
    }
    m_liveUpdate = enabled;
    emit liveUpdateChanged();
}

void EngineController::refreshPorts()
{
    m_serial.refreshPorts();
}

void EngineController::connectToPort(const QString &portName, int baudRate)
{
    m_serial.connectToPort(portName, baudRate);
    if (m_serial.connected()) {
        monitorOn(200);
        QTimer::singleShot(100, this, [this]() { sendCommand(QStringLiteral("status")); });
    }
}

void EngineController::disconnectPort()
{
    if (m_serial.connected()) {
        m_serial.sendLine(QStringLiteral("monitor off"));
    }
    m_serial.disconnectPort();
}

void EngineController::setMode(const QString &mode)
{
    const QString normalized = mode.trimmed().toLower();
    if (normalized != QStringLiteral("adc") && normalized != QStringLiteral("uart")) {
        appendLog(QStringLiteral("ERR"), QStringLiteral("Unsupported engine mode: %1").arg(mode));
        return;
    }
    sendCommand(QStringLiteral("mode %1").arg(normalized));
    setModeLocal(normalized);
}

void EngineController::setThrottle(int value)
{
    setThrottleLocal(percentClamp(value));
    if (!m_applyingRemoteState && m_liveUpdate && m_serial.connected()) {
        m_pedalDebounce.start();
    }
}

void EngineController::setBrake(int value)
{
    setBrakeLocal(percentClamp(value));
    if (!m_applyingRemoteState && m_liveUpdate && m_serial.connected()) {
        m_pedalDebounce.start();
    }
}

void EngineController::applyPedal()
{
    sendCommand(QStringLiteral("pedal %1 %2").arg(m_throttle).arg(m_brake));
    setModeLocal(QStringLiteral("uart"));
}

void EngineController::stop()
{
    setThrottleLocal(0);
    setBrakeLocal(0);
    sendCommand(QStringLiteral("stop"));
}

void EngineController::resetSimulation()
{
    sendCommand(QStringLiteral("sim_reset"));
    QTimer::singleShot(100, this, [this]() { sendCommand(QStringLiteral("status")); });
}

void EngineController::monitorOn(int intervalMs)
{
    sendCommand(QStringLiteral("monitor on %1").arg(qMax(50, intervalMs)));
}

void EngineController::monitorOff()
{
    sendCommand(QStringLiteral("monitor off"));
}

void EngineController::monitorOnce()
{
    sendCommand(QStringLiteral("monitor once"));
}

void EngineController::sendCommand(const QString &command)
{
    m_serial.sendLine(command);
}

void EngineController::processLine(const QString &line)
{
    appendLog(QStringLiteral("RX"), line);

    if (line.contains(QStringLiteral("MON "))) {
        parseMonitorLine(line);
        return;
    }
    parseStatusLine(line);
}

void EngineController::parseMonitorLine(const QString &line)
{
    const int monIndex = line.indexOf(QStringLiteral("MON "));
    if (monIndex < 0) {
        return;
    }

    const QString payload = line.mid(monIndex + 4);
    static const QRegularExpression keyValueRegex(QStringLiteral("(\\w+)=([^\\s]+)"));
    QHash<QString, QString> values;
    QRegularExpressionMatchIterator it = keyValueRegex.globalMatch(payload);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        values.insert(match.captured(1).toLower(), match.captured(2));
    }
    applyValues(values);
}

void EngineController::parseStatusLine(const QString &line)
{
    QString text = line;
    const int promptIndex = text.lastIndexOf(QStringLiteral("CLI >"));
    if (promptIndex >= 0) {
        text = text.mid(promptIndex + 5).trimmed();
    }

    static const QRegularExpression statusRegex(
        QStringLiteral("^(mode|throttle|brake|rpm|speed|coolant|tx_count)\\s*[:=]\\s*([A-Za-z0-9]+)"),
        QRegularExpression::CaseInsensitiveOption);

    const QRegularExpressionMatch match = statusRegex.match(text);
    if (!match.hasMatch()) {
        return;
    }

    QString key = match.captured(1).toLower();
    if (key == QStringLiteral("tx_count")) {
        key = QStringLiteral("tx");
    }
    applyValues({{key, match.captured(2)}});
}

void EngineController::applyValues(const QHash<QString, QString> &values)
{
    bool changed = false;
    const bool previousRemoteState = m_applyingRemoteState;
    m_applyingRemoteState = true;

    if (values.contains(QStringLiteral("mode"))) {
        const QString normalized = values.value(QStringLiteral("mode")).toLower();
        if (m_mode != normalized) {
            m_mode = normalized;
            changed = true;
        }
    }

    bool ok = false;
    if (values.contains(QStringLiteral("throttle"))) {
        const int value = valueToInt(values, QStringLiteral("throttle"), &ok);
        if (ok && m_throttle != value) {
            m_throttle = value;
            changed = true;
        }
    }
    if (values.contains(QStringLiteral("brake"))) {
        const int value = valueToInt(values, QStringLiteral("brake"), &ok);
        if (ok && m_brake != value) {
            m_brake = value;
            changed = true;
        }
    }
    if (values.contains(QStringLiteral("rpm"))) {
        const int value = valueToInt(values, QStringLiteral("rpm"), &ok);
        if (ok && m_rpm != value) {
            m_rpm = value;
            changed = true;
        }
    }
    if (values.contains(QStringLiteral("speed"))) {
        const int value = valueToInt(values, QStringLiteral("speed"), &ok);
        if (ok && m_speed != value) {
            m_speed = value;
            changed = true;
        }
    }
    if (values.contains(QStringLiteral("coolant"))) {
        const int value = valueToInt(values, QStringLiteral("coolant"), &ok);
        if (ok && m_coolant != value) {
            m_coolant = value;
            changed = true;
        }
    }
    if (values.contains(QStringLiteral("tx"))) {
        const int value = valueToInt(values, QStringLiteral("tx"), &ok);
        if (ok && m_txCount != value) {
            m_txCount = value;
            changed = true;
        }
    }

    m_applyingRemoteState = previousRemoteState;
    if (changed) {
        emit dataChanged();
    }
}

void EngineController::appendLog(const QString &direction, const QString &text)
{
    if (m_logModel) {
        m_logModel->append(QStringLiteral("Engine"), direction, text);
    }
}

void EngineController::setModeLocal(const QString &mode)
{
    if (m_mode == mode) {
        return;
    }
    m_mode = mode;
    emit dataChanged();
}

void EngineController::setThrottleLocal(int value)
{
    if (m_throttle == value) {
        return;
    }
    m_throttle = value;
    emit dataChanged();
}

void EngineController::setBrakeLocal(int value)
{
    if (m_brake == value) {
        return;
    }
    m_brake = value;
    emit dataChanged();
}
