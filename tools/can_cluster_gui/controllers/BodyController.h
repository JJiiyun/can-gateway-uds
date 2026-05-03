#ifndef BODYCONTROLLER_H
#define BODYCONTROLLER_H

#include <QObject>

#include "core/SerialDevice.h"

class AppLogModel;

class BodyController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList portNames READ portNames NOTIFY portNamesChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString mode READ mode NOTIFY dataChanged)
    Q_PROPERTY(bool ignition READ ignition NOTIFY dataChanged)
    Q_PROPERTY(bool doorFl READ doorFl NOTIFY dataChanged)
    Q_PROPERTY(bool doorFr READ doorFr NOTIFY dataChanged)
    Q_PROPERTY(bool doorRl READ doorRl NOTIFY dataChanged)
    Q_PROPERTY(bool doorRr READ doorRr NOTIFY dataChanged)
    Q_PROPERTY(bool turnLeft READ turnLeft NOTIFY dataChanged)
    Q_PROPERTY(bool turnRight READ turnRight NOTIFY dataChanged)
    Q_PROPERTY(bool highBeam READ highBeam NOTIFY dataChanged)
    Q_PROPERTY(bool fogLamp READ fogLamp NOTIFY dataChanged)
    Q_PROPERTY(int txCount READ txCount NOTIFY dataChanged)

public:
    explicit BodyController(QObject *parent = nullptr);

    void setLogModel(AppLogModel *logModel);

    QStringList portNames() const;
    bool connected() const;
    QString statusText() const;
    QString mode() const;
    bool ignition() const;
    bool doorFl() const;
    bool doorFr() const;
    bool doorRl() const;
    bool doorRr() const;
    bool turnLeft() const;
    bool turnRight() const;
    bool highBeam() const;
    bool fogLamp() const;
    int txCount() const;

    Q_INVOKABLE void refreshPorts();
    Q_INVOKABLE void connectToPort(const QString &portName, int baudRate = 115200);
    Q_INVOKABLE void disconnectPort();
    Q_INVOKABLE void setMode(const QString &mode);
    Q_INVOKABLE void setIgnition(bool enabled);
    Q_INVOKABLE void setDoorFl(bool enabled);
    Q_INVOKABLE void setDoorFr(bool enabled);
    Q_INVOKABLE void setDoorRl(bool enabled);
    Q_INVOKABLE void setDoorRr(bool enabled);
    Q_INVOKABLE void setTurnLeft(bool enabled);
    Q_INVOKABLE void setTurnRight(bool enabled);
    Q_INVOKABLE void setHighBeam(bool enabled);
    Q_INVOKABLE void setFogLamp(bool enabled);
    Q_INVOKABLE void allDoors(bool open);
    Q_INVOKABLE void sendState();
    Q_INVOKABLE void monitorOn(int intervalMs = 200);
    Q_INVOKABLE void monitorOff();
    Q_INVOKABLE void status();
    Q_INVOKABLE void sendCommand(const QString &command);

signals:
    void portNamesChanged();
    void connectedChanged();
    void statusTextChanged();
    void dataChanged();

private:
    void processLine(const QString &line);
    void parseBodyLine(const QString &line);
    void appendLog(const QString &direction, const QString &text);
    void sendBooleanCommand(const QString &command, bool enabled);
    void setBoolLocal(bool &field, bool value);

    SerialDevice m_serial;
    AppLogModel *m_logModel = nullptr;

    QString m_mode = QStringLiteral("gpio");
    bool m_ignition = false;
    bool m_doorFl = false;
    bool m_doorFr = false;
    bool m_doorRl = false;
    bool m_doorRr = false;
    bool m_turnLeft = false;
    bool m_turnRight = false;
    bool m_highBeam = false;
    bool m_fogLamp = false;
    int m_txCount = 0;
};

#endif // BODYCONTROLLER_H
