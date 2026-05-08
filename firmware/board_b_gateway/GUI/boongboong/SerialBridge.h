#ifndef SERIALBRIDGE_H
#define SERIALBRIDGE_H

#include <QObject>
#include <QSerialPort>
#include <QStringList>
#include <QTimer>

class SerialBridge : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QStringList portNames READ portNames NOTIFY portNamesChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectionChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)

    Q_PROPERTY(int can1Rx READ can1Rx NOTIFY dataChanged)
    Q_PROPERTY(int can1Tx READ can1Tx NOTIFY dataChanged)
    Q_PROPERTY(int can2Rx READ can2Rx NOTIFY dataChanged)
    Q_PROPERTY(int can2Tx READ can2Tx NOTIFY dataChanged)
    Q_PROPERTY(int busy READ busy NOTIFY dataChanged)
    Q_PROPERTY(int errors READ errors NOTIFY dataChanged)
    Q_PROPERTY(bool warning READ warning NOTIFY dataChanged)
    Q_PROPERTY(bool engineRpmWarning READ engineRpmWarning NOTIFY dataChanged)
    Q_PROPERTY(bool engineCoolantWarning READ engineCoolantWarning NOTIFY dataChanged)
    Q_PROPERTY(bool engineGeneralWarning READ engineGeneralWarning NOTIFY dataChanged)
    Q_PROPERTY(int routeMatched READ routeMatched NOTIFY dataChanged)
    Q_PROPERTY(int routeOk READ routeOk NOTIFY dataChanged)
    Q_PROPERTY(int routeFail READ routeFail NOTIFY dataChanged)
    Q_PROPERTY(int routeIgnored READ routeIgnored NOTIFY dataChanged)

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
    Q_PROPERTY(bool clusterSpeedNeedleActive READ clusterSpeedNeedleActive NOTIFY dataChanged)
    Q_PROPERTY(bool clusterCoolantActive READ clusterCoolantActive NOTIFY dataChanged)
    Q_PROPERTY(bool clusterIgnActive READ clusterIgnActive NOTIFY dataChanged)
    Q_PROPERTY(bool clusterBodyActive READ clusterBodyActive NOTIFY dataChanged)
    Q_PROPERTY(bool clusterTurnActive READ clusterTurnActive NOTIFY dataChanged)
    Q_PROPERTY(bool adasValid READ adasValid NOTIFY dataChanged)
    Q_PROPERTY(int adasRisk READ adasRisk NOTIFY dataChanged)
    Q_PROPERTY(int adasFault READ adasFault NOTIFY dataChanged)
    Q_PROPERTY(int adasDtc READ adasDtc NOTIFY dataChanged)
    Q_PROPERTY(int adasFront READ adasFront NOTIFY dataChanged)
    Q_PROPERTY(int adasRear READ adasRear NOTIFY dataChanged)
    Q_PROPERTY(int adasSpeed READ adasSpeed NOTIFY dataChanged)
    Q_PROPERTY(int adasAlive READ adasAlive NOTIFY dataChanged)
    Q_PROPERTY(QString latestFrameId READ latestFrameId NOTIFY dataChanged)
    Q_PROPERTY(QString latestFrameBus READ latestFrameBus NOTIFY dataChanged)
    Q_PROPERTY(QString latestFrameDir READ latestFrameDir NOTIFY dataChanged)
    Q_PROPERTY(QString latestFrameDlc READ latestFrameDlc NOTIFY dataChanged)
    Q_PROPERTY(QString latestFrameRaw READ latestFrameRaw NOTIFY dataChanged)
    Q_PROPERTY(QStringList latestFrameBytes READ latestFrameBytes NOTIFY dataChanged)
    Q_PROPERTY(QString latestFrameDecoded READ latestFrameDecoded NOTIFY dataChanged)

public:
    explicit SerialBridge(QObject *parent = nullptr);

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
    bool engineRpmWarning() const;
    bool engineCoolantWarning() const;
    bool engineGeneralWarning() const;
    int routeMatched() const;
    int routeOk() const;
    int routeFail() const;
    int routeIgnored() const;

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
    bool clusterSpeedNeedleActive() const;
    bool clusterCoolantActive() const;
    bool clusterIgnActive() const;
    bool clusterBodyActive() const;
    bool clusterTurnActive() const;
    bool adasValid() const;
    int adasRisk() const;
    int adasFault() const;
    int adasDtc() const;
    int adasFront() const;
    int adasRear() const;
    int adasSpeed() const;
    int adasAlive() const;
    QString latestFrameId() const;
    QString latestFrameBus() const;
    QString latestFrameDir() const;
    QString latestFrameDlc() const;
    QString latestFrameRaw() const;
    QStringList latestFrameBytes() const;
    QString latestFrameDecoded() const;

    Q_INVOKABLE void refreshPorts();
    Q_INVOKABLE void resetMonitor();
    Q_INVOKABLE void connectToPort(const QString &portName, int baudRate);
    Q_INVOKABLE void disconnectPort();
    Q_INVOKABLE void sendCommand(const QString &command);

signals:
    void portNamesChanged();
    void connectionChanged();
    void statusTextChanged();
    void dataChanged();
    void gatewayLineReceived(const QString &time, const QString &line);
    void frameLineReceived(const QString &time, const QString &bus, const QString &dir,
                           const QString &frameId, const QString &dlc,
                           const QString &payload, const QString &decoded);
    void rawLineReceived(const QString &time, const QString &line);

private slots:
    void onReadyRead();
    void onErrorOccurred(QSerialPort::SerialPortError error);
    void onPollSerial();

private:
    void setStatusText(const QString &text);
    void processLine(const QString &line);
    void parseGatewayStatus(const QString &line);
    void parseFrameLine(const QString &line);
    QString decodeFrame(const QString &id, const QList<int> &bytes, const QString &dir);
    uint32_t leSignal(const QList<int> &bytes, int startBit, int bitLength) const;
    QString nowString() const;
    bool bitAt(const QList<int> &bytes, int bit) const;

    QSerialPort m_serial;
    QTimer m_pollTimer;
    QByteArray m_rxBuffer;
    QStringList m_portNames;
    QString m_statusText;

    int m_can1Rx = 0;
    int m_can1Tx = 0;
    int m_can2Rx = 0;
    int m_can2Tx = 0;
    int m_busy = 0;
    int m_errors = 0;
    bool m_warning = false;
    bool m_engineRpmWarning = false;
    bool m_engineCoolantWarning = false;
    bool m_engineGeneralWarning = false;
    int m_routeMatched = 0;
    int m_routeOk = 0;
    int m_routeFail = 0;
    int m_routeIgnored = 0;

    int m_rpm = 0;
    int m_speed = 0;
    int m_coolant = 0;
    bool m_ignition = false;
    int m_alive = 0;
    QString m_lastEngineRx = "-";

    bool m_doorFl = false;
    bool m_doorFr = false;
    bool m_doorRl = false;
    bool m_doorRr = false;
    bool m_turnLeft = false;
    bool m_turnRight = false;
    bool m_highBeam = false;
    bool m_fogLamp = false;
    QString m_lastBodyRx = "-";

    bool m_clusterRpmActive = false;
    bool m_clusterSpeedActive = false;
    bool m_clusterSpeedNeedleActive = false;
    bool m_clusterCoolantActive = false;
    bool m_clusterIgnActive = false;
    bool m_clusterBodyActive = false;
    bool m_clusterTurnActive = false;
    bool m_adasValid = false;
    int m_adasRisk = 0;
    int m_adasFault = 0;
    int m_adasDtc = 0;
    int m_adasFront = 0;
    int m_adasRear = 0;
    int m_adasSpeed = 0;
    int m_adasAlive = 0;
    QString m_latestFrameId = "-";
    QString m_latestFrameBus = "-";
    QString m_latestFrameDir = "-";
    QString m_latestFrameDlc = "-";
    QString m_latestFrameRaw = "-- -- -- -- -- -- -- --";
    QStringList m_latestFrameBytes = {"--", "--", "--", "--", "--", "--", "--", "--"};
    QString m_latestFrameDecoded = "-";
};

#endif
