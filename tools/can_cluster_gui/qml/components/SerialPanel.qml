import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Panel {
    id: root
    property var device
    property string deviceName: "Device"
    property int defaultBaudIndex: 2
    property bool showReset: false
    signal resetRequested()

    title: deviceName + " Serial"
    subtitle: device ? device.statusText : "No controller"
    height: 128

    RowLayout {
        anchors.fill: parent
        spacing: 12

        ComboBox {
            id: portBox
            model: root.device ? root.device.portNames : []
            Layout.preferredWidth: 210
            Layout.fillHeight: true
        }

        ComboBox {
            id: baudBox
            model: ["9600", "57600", "115200", "230400", "460800", "921600"]
            currentIndex: root.defaultBaudIndex
            Layout.preferredWidth: 126
            Layout.fillHeight: true
        }

        UiButton {
            text: "Refresh"
            Layout.preferredWidth: 100
            onClicked: if (root.device) root.device.refreshPorts()
        }

        UiButton {
            text: root.device && root.device.connected ? "Disconnect" : "Connect"
            success: root.device && root.device.connected
            Layout.preferredWidth: 128
            onClicked: {
                if (!root.device) return
                if (root.device.connected)
                    root.device.disconnectPort()
                else
                    root.device.connectToPort(portBox.currentText, parseInt(baudBox.currentText))
            }
        }

        UiButton {
            visible: root.showReset
            text: "Reset View"
            Layout.preferredWidth: 112
            onClicked: root.resetRequested()
        }

        Item { Layout.fillWidth: true }

        Rectangle {
            Layout.preferredWidth: 112
            Layout.preferredHeight: 32
            radius: 16
            color: root.device && root.device.connected ? "#203b2d" : "#2a2020"
            border.color: root.device && root.device.connected ? "#28c76f" : "#ea5455"
            border.width: 1
            Text {
                anchors.centerIn: parent
                text: root.device && root.device.connected ? "CONNECTED" : "OFFLINE"
                color: "#e8edf2"
                font.pixelSize: 11
                font.weight: Font.Bold
            }
        }
    }
}
