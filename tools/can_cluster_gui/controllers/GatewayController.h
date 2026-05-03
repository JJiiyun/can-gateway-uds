#ifndef GATEWAYCONTROLLER_H
#define GATEWAYCONTROLLER_H

#include <QObject>
#include <QStringList>

#include "core/SerialDevice.h"

class AppLogModel;

class GatewayController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList portNames READ portNames NOTIFY portNamesChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(int can1Rx READ can1Rx NOTIFY dataChanged)
    Q_PROPERTY(int can1Tx READ can1Tx NOTIFY dataChanged)
    Q_PROPERTY(int can2Rx READ can2Rx NOTIFY dataChanged)
    Q_PROPERTY(int can2Tx READ can2Tx NOTIFY dataChanged)
    Q_PROPERTY(int busy READ busy NOTIFY dataChanged)
    Q_PROPERTY(int errors READ errors NOTIFY dataChanged)
    Q_PROPERTY(bool warning READ warning NOTIFY dataChanged)
    Q_PROPERTY(int rpm READ rpm NOTIFY dataChanged)
    Q_PROPERTY(int speed READ speed NOTIFY dataChanged)
    Q_PROPERTY(int coolant READ coolant NOTIFY dataChanged)
    Q_PROPERTY(bool ignition READ ignition NOTIFY dataChanged)
    Q_PROPERTY(int alive READ alive NOTIFY dataChanged)
    Q_PROPERTY(QString lastEngineRx READ lastEngineRx NOTIFY dataChanged)
    Q_PROPERTY(bool doorFl READ doorFl NOTIFY dataChanged)
    Q_PROPERTY(bool doorFr READ doorFr NOTIFY dataChanged)
    Q_PROPERTY(bool doorRl READ doorRl NOTIFY dataChanged)
    Q_PROPERTY(bool doorRr READ doorRr NOTIFY dataChanged)
    Q_PROPERTY(bool turnLeft READ turnLeft NOTIFY dataChanged)
    Q_PROPERTY(bool turnRight READ turnRight NOTIFY dataChanged)
    Q_PROPERTY(bool highBeam READ highBeam NOTIFY dataChanged)
    Q_PROPERTY(bool fogLamp READ fogLamp NOTIFY dataChanged)
    Q_PROPERTY(QString lastBodyRx READ lastBodyRx NOTIFY dataChanged)
    Q_PROPERTY(bool clusterRpmActive READ clusterRpmActive NOTIFY dataChanged)
    Q_PROPERTY(bool clusterSpeedActive READ clusterSpeedActive NOTIFY dataChanged)
    Q_PROPERTY(bool clusterBodyActive READ clusterBodyActive NOTIFY dataChanged)
    Q_PROPERTY(QString latestFrameId READ latestFrameId NOTIFY dataChanged)
    Q_PROPERTY(QString latestFrameBus READ latestFrameBus NOTIFY dataChanged)
    Q_PROPERTY(QString latestFrameDir READ latestFrameDir NOTIFY dataChanged)
    Q_PROPERTY(QString latestFrameDlc READ latestFrameDlc NOTIFY dataChanged)
    Q_PROPERTY(QString latestFrameRaw READ latestFrameRaw NOTIFY dataChanged)
    Q_PROPERTY(QStringList latestFrameBytes READ latestFrameBytes NOTIFY dataChanged)
    Q_PROPERTY(QString latestFrameDecoded READ latestFrameDecoded NOTIFY dataChanged)

public:
    explicit GatewayController(QObject *parent = nullptr);

    void setLogModel(AppLogModel *logModel);

    QStringList portNames() const;
    bool connected() const;
    QString statusText() const;
    int can1Rx() const;
    int can1Tx() const;
    int can2Rx() const;
    int can2Tx() const;
    int busy() const;
    int errors() const;
    bool warning() const;
    int rpm() const;
    int speed() const;
    int coolant() const;
    bool ignition() const;
    int alive() const;
    QString lastEngineRx() const;
    bool doorFl() const;
    bool doorFr() const;
    bool doorRl() const;
    bool doorRr() const;
    bool turnLeft() const;
    bool turnRight() const;
    bool highBeam() const;
    bool fogLamp() const;
    QString lastBodyRx() const;
    bool clusterRpmActive() const;
    bool clusterSpeedActive() const;
    bool clusterBodyActive() const;
    QString latestFrameId() const;
    QString latestFrameBus() const;
    QString latestFrameDir() const;
    QString latestFrameDlc() const;
    QString latestFrameRaw() const;
    QStringList latestFrameBytes() const;
    QString latestFrameDecoded() const;

    Q_INVOKABLE void refreshPorts();
    Q_INVOKABLE void resetMonitor();
    Q_INVOKABLE void connectToPort(const QString &portName, int baudRate = 115200);
    Q_INVOKABLE void disconnectPort();
    Q_INVOKABLE void sendCommand(const QString &command);

signals:
    void portNamesChanged();
    void connectedChanged();
    void statusTextChanged();
    void dataChanged();

private:
    void processLine(const QString &line);
    void parseGatewayStatus(const QString &line);
    void parseFrameLine(const QString &line);
    QString decodeFrame(const QString &id, const QList<int> &bytes, const QString &dir);
    quint32 leSignal(const QList<int> &bytes, int startBit, int bitLength) const;
    bool bitAt(const QList<int> &bytes, int bit) const;
    void appendLog(const QString &direction, const QString &text, const QString &decoded = QString());

    SerialDevice m_serial;
    AppLogModel *m_logModel = nullptr;

    int m_can1Rx = 0;
    int m_can1Tx = 0;
    int m_can2Rx = 0;
    int m_can2Tx = 0;
    int m_busy = 0;
    int m_errors = 0;
    bool m_warning = false;

    int m_rpm = 0;
    int m_speed = 0;
    int m_coolant = 0;
    bool m_ignition = false;
    int m_alive = 0;
    QString m_lastEngineRx = QStringLiteral("-");

    bool m_doorFl = false;
    bool m_doorFr = false;
    bool m_doorRl = false;
    bool m_doorRr = false;
    bool m_turnLeft = false;
    bool m_turnRight = false;
    bool m_highBeam = false;
    bool m_fogLamp = false;
    QString m_lastBodyRx = QStringLiteral("-");

    bool m_clusterRpmActive = false;
    bool m_clusterSpeedActive = false;
    bool m_clusterBodyActive = false;

    QString m_latestFrameId = QStringLiteral("-");
    QString m_latestFrameBus = QStringLiteral("-");
    QString m_latestFrameDir = QStringLiteral("-");
    QString m_latestFrameDlc = QStringLiteral("-");
    QString m_latestFrameRaw = QStringLiteral("-- -- -- -- -- -- -- --");
    QStringList m_latestFrameBytes = {"--", "--", "--", "--", "--", "--", "--", "--"};
    QString m_latestFrameDecoded = QStringLiteral("-");
};

#endif // GATEWAYCONTROLLER_H
