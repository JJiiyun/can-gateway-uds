import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 1180
    height: 760
    minimumWidth: 980
    minimumHeight: 640
    visible: true
    title: qsTr("Board B Gateway Monitor")
    color: "#101418"

    property color panelColor: "#181f26"
    property color panelBorder: "#2a343f"
    property color textPrimary: "#e8edf2"
    property color textMuted: "#8a96a3"
    property color goodColor: "#28c76f"
    property color warnColor: "#ffb020"
    property color badColor: "#ea5455"
    property color accentColor: "#3ba7ff"

    QtObject {
        id: gateway
        property bool connected: false
        property int can1Rx: 1811
        property int can1Tx: 0
        property int can2Rx: 2
        property int can2Tx: 4572
        property int busy: 0
        property int errors: 0
        property bool warning: false
        property int rpm: 1450
        property int speed: 32
        property int coolant: 90
        property bool ignition: true
        property bool doorFl: false
        property bool doorFr: false
        property bool doorRl: true
        property bool doorRr: false
        property bool turnLeft: true
        property bool turnRight: false
        property bool highBeam: false
        property bool fogLamp: true
    }

    Component {
        id: metricCard

        Rectangle {
            property string title: ""
            property string value: ""
            property string detail: ""
            property color accent: root.accentColor

            color: root.panelColor
            border.color: root.panelBorder
            border.width: 1
            radius: 8
            implicitHeight: 116

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true

                    Text {
                        text: title
                        color: root.textMuted
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                    }

                    Rectangle {
                        Layout.alignment: Qt.AlignRight
                        width: 10
                        height: 10
                        radius: 5
                        color: accent
                    }
                }

                Text {
                    text: value
                    color: root.textPrimary
                    font.pixelSize: 30
                    font.weight: Font.Bold
                }

                Text {
                    text: detail
                    color: root.textMuted
                    font.pixelSize: 13
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }
        }
    }

    Component {
        id: statePill

        Rectangle {
            property string label: ""
            property bool active: false

            height: 32
            radius: 6
            color: active ? "#203b2d" : "#202831"
            border.color: active ? root.goodColor : root.panelBorder
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                spacing: 8

                Rectangle {
                    width: 8
                    height: 8
                    radius: 4
                    color: active ? root.goodColor : root.textMuted
                }

                Text {
                    text: label
                    color: root.textPrimary
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                }
            }
        }
    }

    header: Rectangle {
        height: 64
        color: "#0d1115"
        border.color: root.panelBorder
        border.width: 1

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 22
            anchors.rightMargin: 22
            spacing: 14

            Text {
                text: "Board B Gateway Monitor"
                color: root.textPrimary
                font.pixelSize: 22
                font.weight: Font.Bold
            }

            Rectangle {
                Layout.preferredWidth: 1
                Layout.preferredHeight: 28
                color: root.panelBorder
            }

            ComboBox {
                id: portCombo
                Layout.preferredWidth: 230
                model: ["/dev/cu.usbserial", "/dev/cu.usbmodem", "/dev/cu.SLAB_USBtoUART"]
            }

            ComboBox {
                Layout.preferredWidth: 120
                model: ["115200", "921600"]
            }

            Button {
                text: gateway.connected ? "Disconnect" : "Connect"
                onClicked: gateway.connected = !gateway.connected
            }

            Button {
                text: "Refresh"
            }

            Item {
                Layout.fillWidth: true
            }

            Rectangle {
                width: 112
                height: 32
                radius: 16
                color: gateway.connected ? "#203b2d" : "#2a2020"
                border.color: gateway.connected ? root.goodColor : root.badColor
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: gateway.connected ? "CONNECTED" : "OFFLINE"
                    color: root.textPrimary
                    font.pixelSize: 12
                    font.weight: Font.Bold
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 14

        RowLayout {
            Layout.fillWidth: true
            spacing: 14

            Loader {
                Layout.fillWidth: true
                sourceComponent: metricCard
                onLoaded: {
                    item.title = "CAN1 Bus"
                    item.value = "RX " + gateway.can1Rx
                    item.detail = "TX " + gateway.can1Tx + " / Powertrain + Body"
                    item.accent = root.goodColor
                }
            }

            Loader {
                Layout.fillWidth: true
                sourceComponent: metricCard
                onLoaded: {
                    item.title = "CAN2 Bus"
                    item.value = "TX " + gateway.can2Tx
                    item.detail = "RX " + gateway.can2Rx + " / Cluster + Diagnostic"
                    item.accent = root.accentColor
                }
            }

            Loader {
                Layout.fillWidth: true
                sourceComponent: metricCard
                onLoaded: {
                    item.title = "Gateway Health"
                    item.value = gateway.warning ? "WARNING" : "NORMAL"
                    item.detail = "busy " + gateway.busy + " / err " + gateway.errors
                    item.accent = gateway.warning ? root.warnColor : root.goodColor
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 14

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: root.panelColor
                border.color: root.panelBorder
                border.width: 1
                radius: 8

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 14

                    Text {
                        text: "Board A Engine"
                        color: root.textPrimary
                        font.pixelSize: 20
                        font.weight: Font.Bold
                    }

                    Text {
                        text: gateway.rpm + " rpm"
                        color: root.textPrimary
                        font.pixelSize: 42
                        font.weight: Font.Bold
                    }

                    ProgressBar {
                        Layout.fillWidth: true
                        value: gateway.rpm / 7000
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: 18
                        rowSpacing: 12

                        Repeater {
                            model: [
                                ["Speed", gateway.speed + " km/h"],
                                ["Coolant", gateway.coolant + " C"],
                                ["IGN", gateway.ignition ? "ON" : "OFF"],
                                ["Last RX", "24 ms ago"]
                            ]

                            delegate: Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 58
                                radius: 6
                                color: "#202831"

                                Column {
                                    anchors.centerIn: parent
                                    spacing: 3

                                    Text {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        text: modelData[0]
                                        color: root.textMuted
                                        font.pixelSize: 12
                                    }

                                    Text {
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        text: modelData[1]
                                        color: root.textPrimary
                                        font.pixelSize: 18
                                        font.weight: Font.Bold
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: root.panelColor
                border.color: root.panelBorder
                border.width: 1
                radius: 8

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 14

                    Text {
                        text: "Board D Body"
                        color: root.textPrimary
                        font.pixelSize: 20
                        font.weight: Font.Bold
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 4
                        columnSpacing: 8
                        rowSpacing: 8

                        Loader { sourceComponent: statePill; onLoaded: { item.label = "FL Door"; item.active = gateway.doorFl } }
                        Loader { sourceComponent: statePill; onLoaded: { item.label = "FR Door"; item.active = gateway.doorFr } }
                        Loader { sourceComponent: statePill; onLoaded: { item.label = "RL Door"; item.active = gateway.doorRl } }
                        Loader { sourceComponent: statePill; onLoaded: { item.label = "RR Door"; item.active = gateway.doorRr } }
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: 10
                        rowSpacing: 10

                        Loader { sourceComponent: statePill; onLoaded: { item.label = "Left Turn"; item.active = gateway.turnLeft } }
                        Loader { sourceComponent: statePill; onLoaded: { item.label = "Right Turn"; item.active = gateway.turnRight } }
                        Loader { sourceComponent: statePill; onLoaded: { item.label = "High Beam"; item.active = gateway.highBeam } }
                        Loader { sourceComponent: statePill; onLoaded: { item.label = "Fog Lamp"; item.active = gateway.fogLamp } }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: 8
                        color: "#202831"

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 14
                            spacing: 10

                            Text {
                                text: "Cluster / CAN2 Output"
                                color: root.textPrimary
                                font.pixelSize: 16
                                font.weight: Font.Bold
                            }

                            Repeater {
                                model: [
                                    ["0x280", "Motor_1 RPM", "50 ms", true],
                                    ["0x1A0", "Bremse_1 Speed", "500 ms", true],
                                    ["0x390", "Body Forward", "100 ms", true],
                                    ["0x480", "Warning", "event", false]
                                ]

                                delegate: RowLayout {
                                    Layout.fillWidth: true

                                    Text {
                                        text: modelData[0]
                                        color: root.accentColor
                                        font.pixelSize: 14
                                        font.weight: Font.Bold
                                        Layout.preferredWidth: 58
                                    }

                                    Text {
                                        text: modelData[1]
                                        color: root.textPrimary
                                        font.pixelSize: 14
                                        Layout.fillWidth: true
                                    }

                                    Text {
                                        text: modelData[2]
                                        color: root.textMuted
                                        font.pixelSize: 13
                                        Layout.preferredWidth: 70
                                    }

                                    Rectangle {
                                        width: 10
                                        height: 10
                                        radius: 5
                                        color: modelData[3] ? root.goodColor : root.textMuted
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 240
            color: root.panelColor
            border.color: root.panelBorder
            border.width: 1
            radius: 8

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true

                    Text {
                        text: "CAN Log"
                        color: root.textPrimary
                        font.pixelSize: 18
                        font.weight: Font.Bold
                    }

                    Item { Layout.fillWidth: true }

                    Repeater {
                        model: ["All", "0x100", "0x280", "0x1A0", "0x390", "0x480"]

                        delegate: Button {
                            text: modelData
                            checkable: true
                            checked: index === 0
                        }
                    }

                    Button {
                        text: "Clear"
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 6
                    color: "#0d1115"
                    border.color: root.panelBorder

                    ListView {
                        anchors.fill: parent
                        anchors.margins: 8
                        clip: true
                        model: [
                            "12:30:01.120 | CAN1 | RX | 0x100 | 8 | DC 05 20 00 5A 03 00 00 | rpm=1500 speed=32 ign=1",
                            "12:30:01.130 | CAN2 | TX | 0x280 | 8 | 00 00 70 17 00 00 00 00 | cluster rpm=1500",
                            "12:30:01.150 | CAN1 | RX | 0x390 | 8 | 10 00 01 00 00 22 00 04 | door=1 left=1",
                            "12:30:01.151 | CAN2 | TX | 0x390 | 8 | 10 00 01 00 00 22 00 04 | body forward"
                        ]

                        delegate: Text {
                            width: ListView.view.width
                            height: 28
                            text: modelData
                            color: root.textPrimary
                            font.family: "Menlo"
                            font.pixelSize: 12
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }
    }
}
