#ifndef ENGINECONTROLLER_H
#define ENGINECONTROLLER_H

#include <QHash>
#include <QObject>
#include <QTimer>

#include "core/SerialDevice.h"

class AppLogModel;

class EngineController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList portNames READ portNames NOTIFY portNamesChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString mode READ mode NOTIFY dataChanged)
    Q_PROPERTY(int throttle READ throttle NOTIFY dataChanged)
    Q_PROPERTY(int brake READ brake NOTIFY dataChanged)
    Q_PROPERTY(int rpm READ rpm NOTIFY dataChanged)
    Q_PROPERTY(int speed READ speed NOTIFY dataChanged)
    Q_PROPERTY(int coolant READ coolant NOTIFY dataChanged)
    Q_PROPERTY(int txCount READ txCount NOTIFY dataChanged)
    Q_PROPERTY(bool liveUpdate READ liveUpdate WRITE setLiveUpdate NOTIFY liveUpdateChanged)

public:
    explicit EngineController(QObject *parent = nullptr);

    void setLogModel(AppLogModel *logModel);

    QStringList portNames() const;
    bool connected() const;
    QString statusText() const;
    QString mode() const;
    int throttle() const;
    int brake() const;
    int rpm() const;
    int speed() const;
    int coolant() const;
    int txCount() const;
    bool liveUpdate() const;
    void setLiveUpdate(bool enabled);

    Q_INVOKABLE void refreshPorts();
    Q_INVOKABLE void connectToPort(const QString &portName, int baudRate = 115200);
    Q_INVOKABLE void disconnectPort();
    Q_INVOKABLE void setMode(const QString &mode);
    Q_INVOKABLE void setThrottle(int value);
    Q_INVOKABLE void setBrake(int value);
    Q_INVOKABLE void applyPedal();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void resetSimulation();
    Q_INVOKABLE void monitorOn(int intervalMs = 200);
    Q_INVOKABLE void monitorOff();
    Q_INVOKABLE void monitorOnce();
    Q_INVOKABLE void sendCommand(const QString &command);

signals:
    void portNamesChanged();
    void connectedChanged();
    void statusTextChanged();
    void dataChanged();
    void liveUpdateChanged();

private:
    void processLine(const QString &line);
    void parseMonitorLine(const QString &line);
    void parseStatusLine(const QString &line);
    void applyValues(const QHash<QString, QString> &values);
    void appendLog(const QString &direction, const QString &text);
    void setModeLocal(const QString &mode);
    void setThrottleLocal(int value);
    void setBrakeLocal(int value);

    SerialDevice m_serial;
    AppLogModel *m_logModel = nullptr;
    QTimer m_pedalDebounce;
    QString m_mode = QStringLiteral("adc");
    int m_throttle = 0;
    int m_brake = 0;
    int m_rpm = 0;
    int m_speed = 0;
    int m_coolant = 0;
    int m_txCount = 0;
    bool m_liveUpdate = true;
    bool m_applyingRemoteState = false;
};

#endif // ENGINECONTROLLER_H
