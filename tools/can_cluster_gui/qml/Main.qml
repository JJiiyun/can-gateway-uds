import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "components"

ApplicationWindow {
    id: root
    width: 1360
    height: 860
    minimumWidth: 1120
    minimumHeight: 720
    visible: true
    title: qsTr("CAN Cluster Control Center")
    color: "#101418"

    property int currentPage: 0
    readonly property color panelColor: "#181f26"
    readonly property color panelSoft: "#202831"
    readonly property color panelDark: "#0d1115"
    readonly property color panelBorder: "#2a343f"
    readonly property color textPrimary: "#e8edf2"
    readonly property color textMuted: "#8a96a3"
    readonly property color goodColor: "#28c76f"
    readonly property color warnColor: "#ffb020"
    readonly property color badColor: "#ea5455"
    readonly property color accentColor: "#3ba7ff"

    palette.window: root.color
    palette.windowText: root.textPrimary
    palette.base: root.panelDark
    palette.text: root.textPrimary
    palette.button: root.panelSoft
    palette.buttonText: root.textPrimary
    palette.highlight: root.accentColor
    palette.placeholderText: root.textMuted

    Component.onCompleted: {
        engine.refreshPorts()
        gateway.refreshPorts()
        body.refreshPorts()
        uds.refreshPorts()
    }

    component StatusChip: Rectangle {
        property string label: ""
        property bool active: false
        property string sub: ""
        width: 146
        height: 42
        radius: 10
        color: active ? "#203b2d" : "#2a2020"
        border.width: 1
        border.color: active ? root.goodColor : root.badColor
        Rectangle {
            x: 12
            y: parent.height / 2 - 5
            width: 10
            height: 10
            radius: 5
            color: parent.active ? root.goodColor : root.badColor
        }
        Text {
            x: 30
            y: 7
            width: parent.width - 40
            text: label
            color: root.textPrimary
            font.pixelSize: 12
            font.weight: Font.Bold
            elide: Text.ElideRight
        }
        Text {
            x: 30
            y: 23
            width: parent.width - 40
            text: active ? "ONLINE" : "OFFLINE"
            color: root.textMuted
            font.pixelSize: 10
            elide: Text.ElideRight
        }
    }

    component NavButton: Rectangle {
        property string label: ""
        property string icon: ""
        property bool selected: false
        signal clicked()
        width: parent.width - 18
        height: 54
        x: 9
        radius: 10
        color: selected ? "#1f4262" : mouse.containsMouse ? "#202831" : "transparent"
        border.width: selected ? 1 : 0
        border.color: selected ? root.accentColor : "transparent"
        Rectangle {
            x: 0
            y: 10
            width: selected ? 4 : 0
            height: parent.height - 20
            radius: 2
            color: root.accentColor
        }
        Text {
            x: 18
            y: 0
            width: 30
            height: parent.height
            text: icon
            color: selected ? root.accentColor : root.textMuted
            font.pixelSize: 19
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
        }
        Text {
            x: 58
            y: 0
            width: parent.width - 68
            height: parent.height
            text: label
            color: selected ? root.textPrimary : root.textMuted
            font.pixelSize: 14
            font.weight: selected ? Font.Bold : Font.DemiBold
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        MouseArea {
            id: mouse
            anchors.fill: parent
            hoverEnabled: true
            onClicked: parent.clicked()
        }
    }

    Rectangle {
        id: topHeader
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 104
        color: root.panelDark
        border.color: root.panelBorder
        border.width: 1

        Rectangle {
            x: 22
            y: 20
            width: 54
            height: 54
            radius: 14
            color: "#1f4262"
            border.color: root.accentColor
            border.width: 1
            Text {
                anchors.centerIn: parent
                text: "CAN"
                color: root.textPrimary
                font.pixelSize: 13
                font.weight: Font.Black
            }
        }

        Column {
            x: 92
            y: 20
            spacing: 3
            Text {
                text: "CAN Cluster Control Center"
                color: root.textPrimary
                font.pixelSize: 24
                font.weight: Font.Bold
            }
            Text {
                text: "Engine · Body · Gateway · UDS Tester · VW Golf Mk6 Cluster"
                color: root.textMuted
                font.pixelSize: 13
            }
        }

        Row {
            anchors.right: parent.right
            anchors.rightMargin: 22
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10
            StatusChip { label: "ENGINE"; active: engine.connected }
            StatusChip { label: "GATEWAY"; active: gateway.connected }
            StatusChip { label: "BODY"; active: body.connected }
            StatusChip { label: "UDS"; active: uds.connected }
        }
    }

    Rectangle {
        id: sideRail
        anchors.left: parent.left
        anchors.top: topHeader.bottom
        anchors.bottom: parent.bottom
        width: 206
        color: "#0d1115"
        border.color: root.panelBorder
        border.width: 1

        ListModel {
            id: navModel
            ListElement { label: "Dashboard"; icon: "▣" }
            ListElement { label: "Engine ECU"; icon: "⚙" }
            ListElement { label: "Body ECU"; icon: "◆" }
            ListElement { label: "Gateway"; icon: "⇄" }
            ListElement { label: "UDS Tester"; icon: "⌁" }
            ListElement { label: "CAN Log"; icon: "≡" }
        }

        Column {
            anchors.fill: parent
            anchors.topMargin: 18
            spacing: 8
            Repeater {
                model: navModel
                delegate: NavButton {
                    label: model.label
                    icon: model.icon
                    selected: root.currentPage === index
                    onClicked: root.currentPage = index
                }
            }
        }

        Rectangle {
            x: 14
            y: parent.height - 126
            width: parent.width - 28
            height: 96
            radius: 10
            color: "#181f26"
            border.color: "#2a343f"
            border.width: 1
            Text {
                x: 14
                y: 14
                width: parent.width - 28
                text: "Live Log"
                color: root.textMuted
                font.pixelSize: 12
                font.weight: Font.DemiBold
            }
            Text {
                x: 14
                y: 38
                width: parent.width - 28
                text: appLog.count
                color: root.textPrimary
                font.pixelSize: 30
                font.weight: Font.Bold
            }
            Text {
                x: 14
                y: 72
                width: parent.width - 28
                text: "entries captured"
                color: root.textMuted
                font.pixelSize: 11
            }
        }
    }

    StackLayout {
        anchors.left: sideRail.right
        anchors.right: parent.right
        anchors.top: topHeader.bottom
        anchors.bottom: parent.bottom
        currentIndex: root.currentPage

        DashboardPage {}
        EnginePage {}
        BodyPage {}
        GatewayPage {}
        UdsPage {}
        CanLogPage {}
    }
}
