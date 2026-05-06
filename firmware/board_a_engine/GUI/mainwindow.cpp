#include "mainwindow.h"

#include <QAbstractSpinBox>
#include <QComboBox>
#include <QDateTime>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSpinBox>
#include <QStatusBar>
#include <QStyle>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

namespace {

constexpr int kPedalRampMs = 80;
constexpr int kThrottleStep = 3;
constexpr int kBrakeStep = 4;
constexpr int kTargetEngineTxPeriodMs = 50;

QProgressBar *makeProgressBar(int maximum, const QString &format)
{
    auto *bar = new QProgressBar;
    bar->setRange(0, maximum);
    bar->setFormat(format);
    bar->setTextVisible(true);
    bar->setAlignment(Qt::AlignCenter);
    bar->setMinimumHeight(30);
    return bar;
}

QLabel *makeCaption(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setProperty("role", "caption");
    return label;
}

void setButtonRole(QPushButton *button, const char *role)
{
    button->setProperty("role", role);
    button->setCursor(Qt::PointingHandCursor);
}

int statusValue(const QHash<QString, QString> &values,
                const QString &key,
                bool *ok)
{
    const QString raw = values.value(key);
    return raw.toInt(ok);
}

int clampPercent(int value)
{
    if (value < 0) {
        return 0;
    }

    if (value > 100) {
        return 100;
    }

    return value;
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      serial_(new SerialConnection)
{
    setupUi();

    serialPollTimer_.setInterval(20);
    pedalRampTimer_.setInterval(kPedalRampMs);
    monitorApplyTimer_.setInterval(250);
    monitorApplyTimer_.setSingleShot(true);

    connect(&serialPollTimer_, &QTimer::timeout, this, &MainWindow::readSerial);
    connect(&pedalRampTimer_, &QTimer::timeout, this, &MainWindow::updatePedalRamp);
    connect(&monitorApplyTimer_, &QTimer::timeout, this, [this]() {
        applyMonitorInterval(false);
    });

    refreshPorts();
}

MainWindow::~MainWindow()
{
    disconnectSerial();
    delete serial_;
    serial_ = nullptr;
}

void MainWindow::setupUi()
{
    setWindowTitle(QStringLiteral("Engine ECU Simulator"));
    resize(1280, 760);

    auto *central = new QWidget(this);
    central->setObjectName(QStringLiteral("CentralRoot"));

    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(18, 18, 18, 18);
    root->setSpacing(14);
    setCentralWidget(central);

    setStyleSheet(QStringLiteral(
        "QMainWindow, QWidget#CentralRoot {"
        "  background: #0f131a;"
        "  color: #e5e7eb;"
        "  font-family: 'Segoe UI';"
        "  font-size: 13px;"
        "}"

        "QGroupBox {"
        "  background: #171c24;"
        "  border: 1px solid #2b3442;"
        "  border-radius: 16px;"
        "  margin-top: 18px;"
        "  padding: 18px 16px 16px 16px;"
        "  font-weight: 700;"
        "  color: #f3f4f6;"
        "}"

        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  subcontrol-position: top left;"
        "  left: 14px;"
        "  padding: 0 8px;"
        "  color: #9ca3af;"
        "  font-size: 12px;"
        "  font-weight: 800;"
        "}"

        "QLabel[role=\"caption\"] {"
        "  color: #9ca3af;"
        "  font-size: 11px;"
        "  font-weight: 700;"
        "}"

        "QLabel[role=\"pill\"] {"
        "  background: #10151d;"
        "  border: 1px solid #2b3442;"
        "  border-radius: 10px;"
        "  padding: 8px 10px;"
        "  color: #f9fafb;"
        "  font-weight: 700;"
        "}"

        "QComboBox, QSpinBox, QLineEdit {"
        "  background: #10151d;"
        "  border: 1px solid #313b4b;"
        "  border-radius: 9px;"
        "  color: #f9fafb;"
        "  min-height: 30px;"
        "  padding: 4px 8px;"
        "}"

        "QComboBox QAbstractItemView {"
        "  background: #10151d;"
        "  color: #f9fafb;"
        "  selection-background-color: #2563eb;"
        "  selection-color: #ffffff;"
        "  outline: 0;"
        "}"

        "QComboBox#PortCombo {"
        "  background: #0b1220;"
        "  color: #ffffff;"
        "}"

        "QComboBox#PortCombo QAbstractItemView {"
        "  background: #0b1220;"
        "  color: #ffffff;"
        "  selection-background-color: #2563eb;"
        "  selection-color: #ffffff;"
        "}"

        "QComboBox:disabled, QSpinBox:disabled, QLineEdit:disabled {"
        "  color: #6b7280;"
        "  background: #111827;"
        "}"

        "QPushButton {"
        "  background: #222b38;"
        "  border: 1px solid #3b4656;"
        "  border-radius: 10px;"
        "  color: #f9fafb;"
        "  min-height: 32px;"
        "  padding: 5px 12px;"
        "  font-weight: 700;"
        "}"

        "QPushButton:hover {"
        "  background: #2b3544;"
        "  border-color: #4b5563;"
        "}"

        "QPushButton:pressed {"
        "  background: #111827;"
        "  padding-top: 7px;"
        "}"

        "QPushButton[role=\"primary\"] {"
        "  background: #1d4ed8;"
        "  border-color: #2563eb;"
        "}"

        "QPushButton[role=\"primary\"]:hover {"
        "  background: #2563eb;"
        "}"

        "QPushButton[role=\"danger\"] {"
        "  background: #7f1d1d;"
        "  border-color: #b91c1c;"
        "}"

        "QPushButton[role=\"danger\"]:hover {"
        "  background: #991b1b;"
        "}"

        "QPushButton[role=\"stepper\"] {"
        "  min-width: 44px;"
        "  padding-left: 0;"
        "  padding-right: 0;"
        "  font-size: 18px;"
        "}"

        "QRadioButton {"
        "  color: #e5e7eb;"
        "  spacing: 8px;"
        "  font-weight: 700;"
        "}"

        "QTextEdit {"
        "  background: #080b10;"
        "  border: 1px solid #263142;"
        "  border-radius: 14px;"
        "  color: #d1d5db;"
        "  font-family: Consolas, 'Courier New', monospace;"
        "  font-size: 12px;"
        "  padding: 10px;"
        "}"

        "QProgressBar {"
        "  background: #0f141c;"
        "  border: 1px solid #2b3442;"
        "  border-radius: 10px;"
        "  color: #f9fafb;"
        "  font-weight: 800;"
        "  text-align: center;"
        "}"

        "QProgressBar::chunk {"
        "  border-radius: 9px;"
        "  background: #3b82f6;"
        "}"

        "QProgressBar[metric=\"throttle\"]::chunk {"
        "  background: #22c55e;"
        "}"

        "QProgressBar[metric=\"brake\"]::chunk {"
        "  background: #ef4444;"
        "}"

        "QProgressBar[metric=\"rpm\"] {"
        "  font-size: 20px;"
        "}"

        "QProgressBar[metric=\"rpm\"]::chunk {"
        "  background: #f59e0b;"
        "}"

        "QProgressBar[metric=\"speed\"] {"
        "  font-size: 17px;"
        "}"

        "QProgressBar[metric=\"speed\"]::chunk {"
        "  background: #38bdf8;"
        "}"

        "QProgressBar[metric=\"coolant\"]::chunk {"
        "  background: #a855f7;"
        "}"

        "QPushButton#ThrottlePedal, QPushButton#BrakePedal {"
        "  min-height: 190px;"
        "  font-size: 28px;"
        "  font-weight: 900;"
        "  border-radius: 22px;"
        "  color: #f9fafb;"
        "  letter-spacing: 1px;"
        "}"

        "QPushButton#ThrottlePedal {"
        "  background: #18241f;"
        "  border: 2px solid #2f4f3f;"
        "}"

        "QPushButton#ThrottlePedal:hover {"
        "  background: #1d3329;"
        "  border-color: #34d399;"
        "}"

        "QPushButton#ThrottlePedal:pressed, QPushButton#ThrottlePedal[active=\"true\"] {"
        "  background: #064e3b;"
        "  border: 2px solid #34d399;"
        "  padding-top: 12px;"
        "}"

        "QPushButton#BrakePedal {"
        "  background: #2a1719;"
        "  border: 2px solid #5b2c31;"
        "}"

        "QPushButton#BrakePedal:hover {"
        "  background: #3b1d22;"
        "  border-color: #f87171;"
        "}"

        "QPushButton#BrakePedal:pressed, QPushButton#BrakePedal[active=\"true\"] {"
        "  background: #7f1d1d;"
        "  border: 2px solid #f87171;"
        "  padding-top: 12px;"
        "}"

        "QStatusBar {"
        "  background: #0f131a;"
        "  color: #9ca3af;"
        "}"
    ));

    auto *serialGroup = new QGroupBox(QStringLiteral("SERIAL LINK"), central);
    auto *serialLayout = new QGridLayout(serialGroup);
    serialLayout->setContentsMargins(14, 18, 14, 12);
    serialLayout->setHorizontalSpacing(10);
    serialLayout->setVerticalSpacing(8);

    portCombo_ = new QComboBox(serialGroup);
    portCombo_->setObjectName(QStringLiteral("PortCombo"));
    portCombo_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    baudCombo_ = new QComboBox(serialGroup);
    const QList<int> baudRates = {9600, 57600, 115200, 230400, 460800, 921600};
    for (int baud : baudRates) {
        baudCombo_->addItem(QString::number(baud), baud);
    }

    const int defaultBaudIndex = baudCombo_->findData(115200);
    if (defaultBaudIndex >= 0) {
        baudCombo_->setCurrentIndex(defaultBaudIndex);
    }

    refreshPortsButton_ = new QPushButton(style()->standardIcon(QStyle::SP_BrowserReload),
                                          QStringLiteral("Refresh"), serialGroup);
    connectButton_ = new QPushButton(style()->standardIcon(QStyle::SP_DialogApplyButton),
                                     QStringLiteral("Connect"), serialGroup);
    setButtonRole(connectButton_, "primary");

    serialLayout->addWidget(makeCaption(QStringLiteral("PORT"), serialGroup), 0, 0);
    serialLayout->addWidget(portCombo_, 0, 1);
    serialLayout->addWidget(refreshPortsButton_, 0, 2);
    serialLayout->addWidget(makeCaption(QStringLiteral("BAUD"), serialGroup), 0, 3);
    serialLayout->addWidget(baudCombo_, 0, 4);
    serialLayout->addWidget(connectButton_, 0, 5);
    serialLayout->setColumnStretch(1, 1);

    root->addWidget(serialGroup);

    auto *workArea = new QHBoxLayout;
    workArea->setSpacing(14);
    root->addLayout(workArea, 1);

    auto *controlGroup = new QGroupBox(QStringLiteral("ENGINE CONTROL"), central);
    controlGroup->setMinimumWidth(360);
    controlGroup->setMaximumWidth(430);

    auto *controlLayout = new QGridLayout(controlGroup);
    controlLayout->setContentsMargins(16, 22, 16, 16);
    controlLayout->setHorizontalSpacing(12);
    controlLayout->setVerticalSpacing(12);
    controlLayout->setColumnStretch(0, 1);
    controlLayout->setColumnStretch(1, 1);

    adcModeButton_ = new QRadioButton(QStringLiteral("ADC"), controlGroup);
    uartModeButton_ = new QRadioButton(QStringLiteral("UART Pedal"), controlGroup);
    adcModeButton_->setChecked(true);

    auto *modeLayout = new QHBoxLayout;
    modeLayout->setSpacing(12);
    modeLayout->addWidget(makeCaption(QStringLiteral("INPUT MODE"), controlGroup));
    modeLayout->addStretch(1);
    modeLayout->addWidget(adcModeButton_);
    modeLayout->addWidget(uartModeButton_);

    controlLayout->addLayout(modeLayout, 0, 0, 1, 2);

    throttlePedalButton_ = new QPushButton(controlGroup);
    throttlePedalButton_->setObjectName(QStringLiteral("ThrottlePedal"));
    throttlePedalButton_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    brakePedalButton_ = new QPushButton(controlGroup);
    brakePedalButton_->setObjectName(QStringLiteral("BrakePedal"));
    brakePedalButton_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    controlLayout->addWidget(throttlePedalButton_, 1, 0);
    controlLayout->addWidget(brakePedalButton_, 1, 1);

    stopButton_ = new QPushButton(style()->standardIcon(QStyle::SP_MediaStop),
                                  QStringLiteral("Stop"), controlGroup);
    resetButton_ = new QPushButton(style()->standardIcon(QStyle::SP_BrowserReload),
                                   QStringLiteral("Reset"), controlGroup);
    setButtonRole(stopButton_, "danger");

    auto *actionLayout = new QHBoxLayout;
    actionLayout->setSpacing(10);
    actionLayout->addWidget(stopButton_);
    actionLayout->addWidget(resetButton_);

    controlLayout->addLayout(actionLayout, 2, 0, 1, 2);

    monitorIntervalSpin_ = new QSpinBox(controlGroup);
    monitorIntervalSpin_->setRange(100, 5000);
    monitorIntervalSpin_->setSingleStep(50);
    monitorIntervalSpin_->setValue(200);
    monitorIntervalSpin_->setSuffix(QStringLiteral(" ms"));
    monitorIntervalSpin_->setButtonSymbols(QAbstractSpinBox::NoButtons);

    monitorIntervalDownButton_ = new QPushButton(QStringLiteral("-"), controlGroup);
    monitorIntervalUpButton_ = new QPushButton(QStringLiteral("+"), controlGroup);
    setButtonRole(monitorIntervalDownButton_, "stepper");
    setButtonRole(monitorIntervalUpButton_, "stepper");
    monitorIntervalDownButton_->setAutoRepeat(true);
    monitorIntervalUpButton_->setAutoRepeat(true);
    monitorIntervalDownButton_->setAutoRepeatDelay(250);
    monitorIntervalUpButton_->setAutoRepeatDelay(250);
    monitorIntervalDownButton_->setAutoRepeatInterval(80);
    monitorIntervalUpButton_->setAutoRepeatInterval(80);

    monitorOnButton_ = new QPushButton(QStringLiteral("Monitor On"), controlGroup);
    monitorOffButton_ = new QPushButton(QStringLiteral("Off"), controlGroup);
    monitorOnceButton_ = new QPushButton(QStringLiteral("Once"), controlGroup);

    auto *monitorLayout = new QGridLayout;
    monitorLayout->setHorizontalSpacing(8);
    monitorLayout->setVerticalSpacing(8);
    monitorLayout->addWidget(makeCaption(QStringLiteral("MONITOR"), controlGroup), 0, 0);
    monitorLayout->addWidget(monitorIntervalDownButton_, 0, 1);
    monitorLayout->addWidget(monitorIntervalSpin_, 0, 2);
    monitorLayout->addWidget(monitorIntervalUpButton_, 0, 3);
    monitorLayout->addWidget(monitorOnButton_, 1, 0);
    monitorLayout->addWidget(monitorOffButton_, 1, 1);
    monitorLayout->addWidget(monitorOnceButton_, 1, 2);

    controlLayout->addLayout(monitorLayout, 3, 0, 1, 2);
    controlLayout->setRowStretch(4, 1);

    auto *statusGroup = new QGroupBox(QStringLiteral("ENGINE DASHBOARD"), central);
    statusGroup->setMinimumWidth(390);
    statusGroup->setMaximumWidth(480);

    auto *statusLayout = new QGridLayout(statusGroup);
    statusLayout->setContentsMargins(16, 22, 16, 16);
    statusLayout->setHorizontalSpacing(10);
    statusLayout->setVerticalSpacing(12);
    statusLayout->setColumnStretch(1, 1);

    connectionValueLabel_ = new QLabel(QStringLiteral("Disconnected"), statusGroup);
    connectionValueLabel_->setProperty("role", "pill");

    modeValueLabel_ = new QLabel(QStringLiteral("adc"), statusGroup);
    modeValueLabel_->setProperty("role", "pill");

    txCountValueLabel_ = new QLabel(QStringLiteral("0"), statusGroup);
    txCountValueLabel_->setProperty("role", "pill");

    targetTxPeriodValueLabel_ = new QLabel(QStringLiteral("50 ms"), statusGroup);
    targetTxPeriodValueLabel_->setProperty("role", "pill");

    estimatedTxPeriodValueLabel_ = new QLabel(QStringLiteral("--"), statusGroup);
    estimatedTxPeriodValueLabel_->setProperty("role", "pill");

    monitorIntervalValueLabel_ = new QLabel(QStringLiteral("--"), statusGroup);
    monitorIntervalValueLabel_->setProperty("role", "pill");

    statusLayout->addWidget(makeCaption(QStringLiteral("CONNECTION"), statusGroup), 0, 0);
    statusLayout->addWidget(connectionValueLabel_, 0, 1);
    statusLayout->addWidget(makeCaption(QStringLiteral("MODE"), statusGroup), 1, 0);
    statusLayout->addWidget(modeValueLabel_, 1, 1);
    statusLayout->addWidget(makeCaption(QStringLiteral("TX COUNT"), statusGroup), 2, 0);
    statusLayout->addWidget(txCountValueLabel_, 2, 1);
    statusLayout->addWidget(makeCaption(QStringLiteral("TARGET TX PERIOD"), statusGroup), 3, 0);
    statusLayout->addWidget(targetTxPeriodValueLabel_, 3, 1);
    statusLayout->addWidget(makeCaption(QStringLiteral("EST. TX PERIOD"), statusGroup), 4, 0);
    statusLayout->addWidget(estimatedTxPeriodValueLabel_, 4, 1);
    statusLayout->addWidget(makeCaption(QStringLiteral("MONITOR INTERVAL"), statusGroup), 5, 0);
    statusLayout->addWidget(monitorIntervalValueLabel_, 5, 1);

    rpmBar_ = makeProgressBar(6000, QStringLiteral("%v rpm"));
    rpmBar_->setProperty("metric", "rpm");
    rpmBar_->setMinimumHeight(66);

    speedBar_ = makeProgressBar(130, QStringLiteral("%v km/h"));
    speedBar_->setProperty("metric", "speed");
    speedBar_->setMinimumHeight(52);

    coolantBar_ = makeProgressBar(120, QStringLiteral("%v C"));
    coolantBar_->setProperty("metric", "coolant");
    coolantBar_->setMinimumHeight(42);

    throttleBar_ = makeProgressBar(100, QStringLiteral("Throttle  %v%"));
    throttleBar_->setProperty("metric", "throttle");
    throttleBar_->setMinimumHeight(38);

    brakeBar_ = makeProgressBar(100, QStringLiteral("Brake  %v%"));
    brakeBar_->setProperty("metric", "brake");
    brakeBar_->setMinimumHeight(38);

    statusLayout->addWidget(makeCaption(QStringLiteral("RPM"), statusGroup), 6, 0);
    statusLayout->addWidget(rpmBar_, 6, 1);
    statusLayout->addWidget(makeCaption(QStringLiteral("SPEED"), statusGroup), 7, 0);
    statusLayout->addWidget(speedBar_, 7, 1);
    statusLayout->addWidget(makeCaption(QStringLiteral("COOLANT"), statusGroup), 8, 0);
    statusLayout->addWidget(coolantBar_, 8, 1);
    statusLayout->addWidget(makeCaption(QStringLiteral("THROTTLE"), statusGroup), 9, 0);
    statusLayout->addWidget(throttleBar_, 9, 1);
    statusLayout->addWidget(makeCaption(QStringLiteral("BRAKE"), statusGroup), 10, 0);
    statusLayout->addWidget(brakeBar_, 10, 1);
    statusLayout->setRowStretch(11, 1);

    auto *consoleGroup = new QGroupBox(QStringLiteral("SERIAL CONSOLE"), central);
    auto *consoleLayout = new QVBoxLayout(consoleGroup);
    consoleLayout->setContentsMargins(16, 22, 16, 16);
    consoleLayout->setSpacing(10);

    terminalLog_ = new QTextEdit(consoleGroup);
    terminalLog_->setReadOnly(true);
    terminalLog_->document()->setMaximumBlockCount(1200);

    commandEdit_ = new QLineEdit(consoleGroup);
    commandEdit_->setPlaceholderText(QStringLiteral("CLI command, ex) status / mode uart / pedal 50 0"));

    sendCommandButton_ = new QPushButton(QStringLiteral("Send"), consoleGroup);
    setButtonRole(sendCommandButton_, "primary");

    auto *commandLayout = new QHBoxLayout;
    commandLayout->setSpacing(8);
    commandLayout->addWidget(commandEdit_, 1);
    commandLayout->addWidget(sendCommandButton_);

    consoleLayout->addWidget(terminalLog_, 1);
    consoleLayout->addLayout(commandLayout);

    workArea->addWidget(controlGroup, 0);
    workArea->addWidget(statusGroup, 0);
    workArea->addWidget(consoleGroup, 1);

    connect(refreshPortsButton_, &QPushButton::clicked, this, &MainWindow::refreshPorts);
    connect(connectButton_, &QPushButton::clicked, this, &MainWindow::toggleConnection);
    connect(sendCommandButton_, &QPushButton::clicked, this, &MainWindow::sendCustomCommand);
    connect(commandEdit_, &QLineEdit::returnPressed, this, &MainWindow::sendCustomCommand);

    connect(adcModeButton_, &QRadioButton::clicked, this, [this]() {
        if (!updatingControls_ && sendCommand(QStringLiteral("mode adc"))) {
            throttlePedalDown_ = false;
            brakePedalDown_ = false;
            pedalRampTimer_.stop();
            throttleCommand_ = 0;
            brakeCommand_ = 0;
            setModeControls(QStringLiteral("adc"));
            updatePedalButtons();
        }
    });

    connect(uartModeButton_, &QRadioButton::clicked, this, [this]() {
        if (!updatingControls_ && sendCommand(QStringLiteral("mode uart"))) {
            setModeControls(QStringLiteral("uart"));
        }
    });

    connect(throttlePedalButton_, &QPushButton::pressed, this, &MainWindow::pressThrottlePedal);
    connect(throttlePedalButton_, &QPushButton::released, this, &MainWindow::releaseThrottlePedal);
    connect(brakePedalButton_, &QPushButton::pressed, this, &MainWindow::pressBrakePedal);
    connect(brakePedalButton_, &QPushButton::released, this, &MainWindow::releaseBrakePedal);

    connect(stopButton_, &QPushButton::clicked, this, [this]() {
        throttlePedalDown_ = false;
        brakePedalDown_ = false;
        pedalRampTimer_.stop();
        throttleCommand_ = 0;
        brakeCommand_ = 0;
        updatePedalButtons();

        if (sendCommand(QStringLiteral("stop"))) {
            setModeControls(QStringLiteral("uart"));
        }
    });

    connect(resetButton_, &QPushButton::clicked, this, [this]() {
        throttlePedalDown_ = false;
        brakePedalDown_ = false;
        pedalRampTimer_.stop();
        throttleCommand_ = 0;
        brakeCommand_ = 0;
        updatePedalButtons();

        sendCommand(QStringLiteral("sim_reset"));

        QTimer::singleShot(100, this, [this]() {
            sendCommand(QStringLiteral("status"), false);
        });
    });

    connect(monitorOnButton_, &QPushButton::clicked, this, [this]() {
        applyMonitorInterval(true);
    });

    connect(monitorOffButton_, &QPushButton::clicked, this, [this]() {
        monitorApplyTimer_.stop();
        if (sendCommand(QStringLiteral("monitor off"))) {
            monitorStreaming_ = false;
        }
    });

    connect(monitorOnceButton_, &QPushButton::clicked, this, [this]() {
        sendCommand(QStringLiteral("monitor once"));
    });

    connect(monitorIntervalDownButton_, &QPushButton::clicked, this, [this]() {
        monitorIntervalSpin_->stepDown();
    });

    connect(monitorIntervalUpButton_, &QPushButton::clicked, this, [this]() {
        monitorIntervalSpin_->stepUp();
    });

    connect(monitorIntervalSpin_, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int) {
                queueMonitorIntervalApply();
            });

    updatePedalButtons();
    statusBar()->showMessage(QStringLiteral("Disconnected"));
}

void MainWindow::refreshPorts()
{
    const QString previousPort = portCombo_->currentData().toString();

    portCombo_->clear();

    const QStringList ports = serial_->availablePorts();
    for (const QString &portName : ports) {
        portCombo_->addItem(portName, portName);
    }

    if (ports.isEmpty()) {
        portCombo_->addItem(QStringLiteral("No serial ports"), QString());
        portCombo_->setEnabled(false);
        appendLog(QStringLiteral("SYS"), QStringLiteral("No serial ports found"));
        return;
    }

    portCombo_->setEnabled(!isSerialOpen());

    const int previousIndex = portCombo_->findData(previousPort);
    if (previousIndex >= 0) {
        portCombo_->setCurrentIndex(previousIndex);
    }
}

void MainWindow::toggleConnection()
{
    if (isSerialOpen()) {
        disconnectSerial();
    } else {
        connectSerial();
    }
}

void MainWindow::connectSerial()
{
    const QString portName = portCombo_->currentData().toString();
    if (portName.isEmpty()) {
        appendLog(QStringLiteral("SYS"), QStringLiteral("Select a serial port first"));
        return;
    }

    if (!serial_->open(portName, baudCombo_->currentData().toInt())) {
        appendLog(QStringLiteral("SYS"), serial_->errorString());
        setConnectionState(QStringLiteral("Disconnected"));
        return;
    }

    rxBuffer_.clear();
    monitorApplyTimer_.stop();
    lastMonitorMs_ = 0;
    lastMonitorTxCount_ = 0;
    txPeriodEstimateValid_ = false;
    silentPedalResponseLines_ = 0;
    silentPedalFilterUntilMs_ = 0;
    monitorStreaming_ = false;
    txCountValueLabel_->setText(QStringLiteral("0"));
    targetTxPeriodValueLabel_->setText(QStringLiteral("%1 ms").arg(kTargetEngineTxPeriodMs));
    estimatedTxPeriodValueLabel_->setText(QStringLiteral("--"));
    monitorIntervalValueLabel_->setText(QStringLiteral("--"));
    serialPollTimer_.start();

    portCombo_->setEnabled(false);
    baudCombo_->setEnabled(false);
    refreshPortsButton_->setEnabled(false);
    connectButton_->setText(QStringLiteral("Disconnect"));
    connectButton_->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));
    setConnectionState(QStringLiteral("Connected to %1").arg(portName));
    appendLog(QStringLiteral("SYS"), QStringLiteral("Connected to %1 at %2 baud")
                  .arg(portName)
                  .arg(baudCombo_->currentData().toInt()));

    applyMonitorInterval(false);
    QTimer::singleShot(100, this, [this]() {
        sendCommand(QStringLiteral("status"), false);
    });
}

void MainWindow::disconnectSerial()
{
    if (!serial_ || !serial_->isOpen()) {
        return;
    }

    serialPollTimer_.stop();
    monitorApplyTimer_.stop();
    serial_->write("monitor off\r\n");
    serial_->close();
    silentPedalResponseLines_ = 0;
    silentPedalFilterUntilMs_ = 0;
    monitorStreaming_ = false;

    portCombo_->setEnabled(true);
    baudCombo_->setEnabled(true);
    refreshPortsButton_->setEnabled(true);
    connectButton_->setText(QStringLiteral("Connect"));
    connectButton_->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
    setConnectionState(QStringLiteral("Disconnected"));
    appendLog(QStringLiteral("SYS"), QStringLiteral("Disconnected"));
}

void MainWindow::readSerial()
{
    if (!serial_ || !serial_->isOpen()) {
        return;
    }

    const QByteArray data = serial_->readAll();
    if (serial_->hasError()) {
        handleSerialError(serial_->errorString());
        return;
    }

    if (data.isEmpty()) {
        return;
    }

    rxBuffer_.append(data);

    int newlineIndex = rxBuffer_.indexOf('\n');
    while (newlineIndex >= 0) {
        const QByteArray rawLine = rxBuffer_.left(newlineIndex);
        rxBuffer_.remove(0, newlineIndex + 1);

        const QString line = QString::fromUtf8(rawLine).trimmed();
        if (!line.isEmpty()) {
            handleLine(line);
        }

        newlineIndex = rxBuffer_.indexOf('\n');
    }
}

void MainWindow::handleSerialError(const QString &message)
{
    appendLog(QStringLiteral("ERR"), message);

    if (serial_) {
        serialPollTimer_.stop();
        monitorApplyTimer_.stop();
        serial_->close();
    }
    silentPedalResponseLines_ = 0;
    silentPedalFilterUntilMs_ = 0;
    monitorStreaming_ = false;

    portCombo_->setEnabled(true);
    baudCombo_->setEnabled(true);
    refreshPortsButton_->setEnabled(true);
    connectButton_->setText(QStringLiteral("Connect"));
    connectButton_->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
    setConnectionState(QStringLiteral("Disconnected"));
}

bool MainWindow::sendCommand(const QString &command, bool logTx)
{
    const QString trimmed = command.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }

    if (logTx) {
        silentPedalResponseLines_ = 0;
        silentPedalFilterUntilMs_ = 0;
    }

    if (!isSerialOpen()) {
        if (logTx) {
            appendLog(QStringLiteral("SYS"), QStringLiteral("Not connected: %1").arg(trimmed));
        }
        return false;
    }

    QByteArray payload = trimmed.toUtf8();
    payload.append("\r\n");
    if (serial_->write(payload) < 0) {
        handleSerialError(serial_->errorString());
        return false;
    }

    if (logTx) {
        appendLog(QStringLiteral("TX"), trimmed);
    }

    return true;
}

void MainWindow::sendPedal(bool logTx)
{
    if (sendCommand(QStringLiteral("pedal %1 %2")
                        .arg(throttleCommand_)
                        .arg(brakeCommand_),
                    logTx)) {
        if (!logTx) {
            silentPedalResponseLines_ += 8;
            silentPedalFilterUntilMs_ = QDateTime::currentMSecsSinceEpoch() + 1500;
            if (silentPedalResponseLines_ > 120) {
                silentPedalResponseLines_ = 120;
            }
        }

        setModeControls(QStringLiteral("uart"));
    }
}

void MainWindow::sendCustomCommand()
{
    const QString command = commandEdit_->text().trimmed();
    if (command.isEmpty()) {
        return;
    }

    silentPedalResponseLines_ = 0;
    silentPedalFilterUntilMs_ = 0;
    sendCommand(command);
    commandEdit_->clear();
}

void MainWindow::pressThrottlePedal()
{
    throttlePedalDown_ = true;
    if (throttleCommand_ < 8) {
        throttleCommand_ = 8;
    }

    updatePedalButtons();
    sendPedal(false);
    pedalRampTimer_.start();
}

void MainWindow::releaseThrottlePedal()
{
    throttlePedalDown_ = false;
    throttleCommand_ = 0;
    updatePedalButtons();
    sendPedal(false);

    if (!brakePedalDown_) {
        pedalRampTimer_.stop();
    }
}

void MainWindow::pressBrakePedal()
{
    brakePedalDown_ = true;
    if (brakeCommand_ < 10) {
        brakeCommand_ = 10;
    }

    updatePedalButtons();
    sendPedal(false);
    pedalRampTimer_.start();
}

void MainWindow::releaseBrakePedal()
{
    brakePedalDown_ = false;
    brakeCommand_ = 0;
    updatePedalButtons();
    sendPedal(false);

    if (!throttlePedalDown_) {
        pedalRampTimer_.stop();
    }
}

void MainWindow::updatePedalRamp()
{
    bool changed = false;

    if (throttlePedalDown_ && throttleCommand_ < 100) {
        throttleCommand_ = clampPercent(throttleCommand_ + kThrottleStep);
        changed = true;
    }

    if (brakePedalDown_ && brakeCommand_ < 100) {
        brakeCommand_ = clampPercent(brakeCommand_ + kBrakeStep);
        changed = true;
    }

    if (!changed) {
        return;
    }

    updatePedalButtons();
    sendPedal(false);
}

void MainWindow::applyMonitorInterval(bool logTx)
{
    monitorApplyTimer_.stop();

    if (!isSerialOpen()) {
        return;
    }

    lastMonitorMs_ = 0;
    lastMonitorTxCount_ = 0;
    txPeriodEstimateValid_ = false;
    estimatedTxPeriodValueLabel_->setText(QStringLiteral("--"));
    monitorIntervalValueLabel_->setText(QStringLiteral("--"));

    if (sendCommand(QStringLiteral("monitor on %1").arg(monitorIntervalSpin_->value()),
                    logTx)) {
        monitorStreaming_ = true;
    }
}

void MainWindow::queueMonitorIntervalApply()
{
    if (!isSerialOpen() || !monitorStreaming_) {
        return;
    }

    monitorApplyTimer_.start();
}

void MainWindow::handleLine(const QString &line)
{
    if (line.contains(QStringLiteral("MON "))) {
        parseMonitorLine(line);
        appendLog(QStringLiteral("RX"), line);
        return;
    }

    parseStatusLine(line);

    if (shouldHideSilentPedalResponse(line)) {
        return;
    }

    appendLog(QStringLiteral("RX"), line);
}

void MainWindow::parseMonitorLine(const QString &line)
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

    applyStatusValues(values);
}

void MainWindow::parseStatusLine(const QString &line)
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

    QHash<QString, QString> values;
    values.insert(key, match.captured(2));
    applyStatusValues(values);
}

void MainWindow::applyStatusValues(const QHash<QString, QString> &values)
{
    const bool wasUpdating = updatingControls_;
    updatingControls_ = true;

    if (values.contains(QStringLiteral("mode"))) {
        setModeControls(values.value(QStringLiteral("mode")).toLower());
    }

    bool ok = false;
    if (values.contains(QStringLiteral("throttle"))) {
        const int value = statusValue(values, QStringLiteral("throttle"), &ok);
        if (ok) {
            throttleBar_->setValue(value);
            if (currentMode_ == QStringLiteral("uart") && !throttlePedalDown_) {
                setThrottleControl(value);
            }
        }
    }

    if (values.contains(QStringLiteral("brake"))) {
        const int value = statusValue(values, QStringLiteral("brake"), &ok);
        if (ok) {
            brakeBar_->setValue(value);
            if (currentMode_ == QStringLiteral("uart") && !brakePedalDown_) {
                setBrakeControl(value);
            }
        }
    }

    if (values.contains(QStringLiteral("rpm"))) {
        const int value = statusValue(values, QStringLiteral("rpm"), &ok);
        if (ok) {
            rpmBar_->setValue(value);
        }
    }

    if (values.contains(QStringLiteral("speed"))) {
        const int value = statusValue(values, QStringLiteral("speed"), &ok);
        if (ok) {
            speedBar_->setValue(value);
        }
    }

    if (values.contains(QStringLiteral("coolant"))) {
        const int value = statusValue(values, QStringLiteral("coolant"), &ok);
        if (ok) {
            coolantBar_->setValue(value);
        }
    }

    if (values.contains(QStringLiteral("tx"))) {
        const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
        const QString txValue = values.value(QStringLiteral("tx"));
        const uint32_t txCount = txValue.toUInt(&ok);

        txCountValueLabel_->setText(txValue);

        if (lastMonitorMs_ > 0) {
            const qint64 elapsedMs = nowMs - lastMonitorMs_;

            if (elapsedMs >= 0) {
                monitorIntervalValueLabel_->setText(QStringLiteral("%1 ms").arg(elapsedMs));
            }

            if (ok && txCount >= lastMonitorTxCount_) {
                const uint32_t deltaTx = txCount - lastMonitorTxCount_;

                if (deltaTx > 0U && elapsedMs > 0) {
                    const double estimatedPeriod = (double)elapsedMs / (double)deltaTx;
                    estimatedTxPeriodValueLabel_->setText(QStringLiteral("%1 ms").arg(estimatedPeriod, 0, 'f', 1));
                    txPeriodEstimateValid_ = true;
                }
            }
        }

        if (ok) {
            lastMonitorTxCount_ = txCount;
        }

        lastMonitorMs_ = nowMs;

        if (!txPeriodEstimateValid_) {
            estimatedTxPeriodValueLabel_->setText(QStringLiteral("--"));
        }
    }

    updatingControls_ = wasUpdating;
}

bool MainWindow::shouldHideSilentPedalResponse(const QString &line)
{
    if (silentPedalResponseLines_ <= 0) {
        return false;
    }

    if (QDateTime::currentMSecsSinceEpoch() > silentPedalFilterUntilMs_) {
        silentPedalResponseLines_ = 0;
        silentPedalFilterUntilMs_ = 0;
        return false;
    }

    QString text = line;
    const int promptIndex = text.lastIndexOf(QStringLiteral("CLI >"));
    if (promptIndex >= 0) {
        text = text.mid(promptIndex + 5).trimmed();
    }

    static const QRegularExpression pedalEchoFragmentRegex(
        QStringLiteral("^(?:p|pe|ped|peda|pedal|edal|dal|al|l)?\\s*\\d{1,3}(?:\\s+\\d{1,3})?\\s*$"),
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression pedalStatusFragmentRegex(
        QStringLiteral("^(?:mode|ode|de|e|throttle|hrottle|rottle|ottle|ttle|tle|le|brake|rake|ake|ke)\\s*[:=]"),
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression pedalValueFragmentRegex(
        QStringLiteral("^(?:uart|art|rt|t|\\d{1,3}\\s*%)\\s*$"),
        QRegularExpression::CaseInsensitiveOption);

    const bool isExpectedPedalResponse =
        text.startsWith(QStringLiteral("pedal "), Qt::CaseInsensitive) ||
        text.startsWith(QStringLiteral("mode "), Qt::CaseInsensitive) ||
        text.startsWith(QStringLiteral("throttle "), Qt::CaseInsensitive) ||
        text.startsWith(QStringLiteral("brake "), Qt::CaseInsensitive) ||
        pedalEchoFragmentRegex.match(text).hasMatch() ||
        pedalStatusFragmentRegex.match(text).hasMatch() ||
        pedalValueFragmentRegex.match(text).hasMatch();

    if (!isExpectedPedalResponse) {
        return false;
    }

    --silentPedalResponseLines_;
    return true;
}

void MainWindow::setConnectionState(const QString &state)
{
    connectionValueLabel_->setText(state);
    statusBar()->showMessage(state);
}

void MainWindow::setModeControls(const QString &mode)
{
    const QString normalized = mode.toLower();
    currentMode_ = normalized;
    modeValueLabel_->setText(normalized);

    QSignalBlocker adcBlocker(adcModeButton_);
    QSignalBlocker uartBlocker(uartModeButton_);
    adcModeButton_->setChecked(normalized == QStringLiteral("adc"));
    uartModeButton_->setChecked(normalized == QStringLiteral("uart"));

    if (normalized == QStringLiteral("adc") && !throttlePedalDown_ && !brakePedalDown_) {
        throttleCommand_ = 0;
        brakeCommand_ = 0;
        updatePedalButtons();
    }
}

void MainWindow::setThrottleControl(int value)
{
    throttleCommand_ = clampPercent(value);
    updatePedalButtons();
}

void MainWindow::setBrakeControl(int value)
{
    brakeCommand_ = clampPercent(value);
    updatePedalButtons();
}

void MainWindow::updatePedalButtons()
{
    throttlePedalButton_->setText(QStringLiteral("ACCEL\n%1%").arg(throttleCommand_));
    brakePedalButton_->setText(QStringLiteral("BRAKE\n%1%").arg(brakeCommand_));

    throttlePedalButton_->setProperty("active", throttlePedalDown_);
    brakePedalButton_->setProperty("active", brakePedalDown_);

    throttlePedalButton_->style()->unpolish(throttlePedalButton_);
    throttlePedalButton_->style()->polish(throttlePedalButton_);
    brakePedalButton_->style()->unpolish(brakePedalButton_);
    brakePedalButton_->style()->polish(brakePedalButton_);
}

void MainWindow::appendLog(const QString &direction, const QString &text)
{
    const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz"));
    QTextCursor cursor = terminalLog_->textCursor();
    cursor.movePosition(QTextCursor::End);

    if (!terminalLog_->document()->isEmpty()) {
        cursor.insertBlock();
    }

    if (direction == QStringLiteral("RX")) {
        cursor.insertText(text);
        terminalLog_->setTextCursor(cursor);
        terminalLog_->ensureCursorVisible();
        return;
    }

    if (direction == QStringLiteral("TX")) {
        cursor.insertHtml(QStringLiteral(
            "<span style=\"color:#60a5fa;font-weight:700;\">TX</span> "
            "<span style=\"color:#dbeafe;\">%1</span>")
                                 .arg(text.toHtmlEscaped()));
        terminalLog_->setTextCursor(cursor);
        terminalLog_->ensureCursorVisible();
        return;
    }

    const QString color = direction == QStringLiteral("ERR")
                              ? QStringLiteral("#f87171")
                              : QStringLiteral("#9ca3af");
    cursor.insertHtml(QStringLiteral(
        "<span style=\"color:#6b7280;\">[%1]</span> "
        "<span style=\"color:%2;font-weight:700;\">%3</span> "
        "<span style=\"color:#d1d5db;\">%4</span>")
                             .arg(timestamp,
                                  color,
                                  direction.toHtmlEscaped(),
                                  text.toHtmlEscaped()));
    terminalLog_->setTextCursor(cursor);
    terminalLog_->ensureCursorVisible();
}

bool MainWindow::isSerialOpen() const
{
    return serial_ && serial_->isOpen();
}
