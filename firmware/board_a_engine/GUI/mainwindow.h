#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QByteArray>
#include <QHash>
#include <QMainWindow>
#include <QString>
#include <QTimer>

#include "serialconnection.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QRadioButton;
class QSpinBox;
class QTextEdit;

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void setupUi();
    void refreshPorts();
    void toggleConnection();
    void connectSerial();
    void disconnectSerial();
    void readSerial();
    void handleSerialError(const QString &message);

    bool sendCommand(const QString &command, bool logTx = true);
    void sendPedal(bool logTx = false);
    void sendCustomCommand();

    void pressThrottlePedal();
    void releaseThrottlePedal();
    void pressBrakePedal();
    void releaseBrakePedal();
    void updatePedalRamp();
    void applyMonitorInterval(bool logTx);
    void queueMonitorIntervalApply();

    void handleLine(const QString &line);
    void parseMonitorLine(const QString &line);
    void parseStatusLine(const QString &line);
    void applyStatusValues(const QHash<QString, QString> &values);
    bool shouldHideSilentPedalResponse(const QString &line);

    void setConnectionState(const QString &state);
    void setModeControls(const QString &mode);
    void setThrottleControl(int value);
    void setBrakeControl(int value);
    void updatePedalButtons();
    void appendLog(const QString &direction, const QString &text);
    bool isSerialOpen() const;

    SerialConnection *serial_ = nullptr;
    QByteArray rxBuffer_;
    QTimer serialPollTimer_;
    QTimer pedalRampTimer_;
    QTimer monitorApplyTimer_;
    QString currentMode_ = QStringLiteral("adc");
    qint64 lastMonitorMs_ = 0;
    uint32_t lastMonitorTxCount_ = 0;
    bool txPeriodEstimateValid_ = false;
    int silentPedalResponseLines_ = 0;
    qint64 silentPedalFilterUntilMs_ = 0;
    int throttleCommand_ = 0;
    int brakeCommand_ = 0;
    bool throttlePedalDown_ = false;
    bool brakePedalDown_ = false;
    bool updatingControls_ = false;
    bool monitorStreaming_ = false;

    QComboBox *portCombo_ = nullptr;
    QComboBox *baudCombo_ = nullptr;
    QPushButton *refreshPortsButton_ = nullptr;
    QPushButton *connectButton_ = nullptr;

    QRadioButton *adcModeButton_ = nullptr;
    QRadioButton *uartModeButton_ = nullptr;
    QPushButton *throttlePedalButton_ = nullptr;
    QPushButton *brakePedalButton_ = nullptr;
    QSpinBox *monitorIntervalSpin_ = nullptr;
    QPushButton *monitorIntervalDownButton_ = nullptr;
    QPushButton *monitorIntervalUpButton_ = nullptr;
    QPushButton *stopButton_ = nullptr;
    QPushButton *resetButton_ = nullptr;
    QPushButton *monitorOnButton_ = nullptr;
    QPushButton *monitorOffButton_ = nullptr;
    QPushButton *monitorOnceButton_ = nullptr;

    QLabel *connectionValueLabel_ = nullptr;
    QLabel *modeValueLabel_ = nullptr;
    QLabel *targetTxPeriodValueLabel_ = nullptr;
    QLabel *estimatedTxPeriodValueLabel_ = nullptr;
    QLabel *monitorIntervalValueLabel_ = nullptr;
    QProgressBar *throttleBar_ = nullptr;
    QProgressBar *brakeBar_ = nullptr;
    QProgressBar *rpmBar_ = nullptr;
    QProgressBar *speedBar_ = nullptr;
    QProgressBar *coolantBar_ = nullptr;
    QLabel *txCountValueLabel_ = nullptr;

    QTextEdit *terminalLog_ = nullptr;
    QLineEdit *commandEdit_ = nullptr;
    QPushButton *sendCommandButton_ = nullptr;
};

#endif
