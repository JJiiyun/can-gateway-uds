import QtQuick
import QtQuick.Controls
import "components"

Rectangle {
    id: root
    color: "#101418"

    Flickable {
        id: flick
        anchors.fill: parent
        contentWidth: Math.max(width, 1080)
        contentHeight: 920
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        Item {
            id: page
            width: flick.contentWidth
            height: flick.contentHeight
            readonly property int margin: 18
            readonly property int gap: 14
            readonly property int innerWidth: width - margin * 2
            readonly property int halfW: (innerWidth - gap) / 2
            readonly property int thirdW: (innerWidth - gap * 2) / 3

            SerialPanel {
                x: page.margin
                y: 18
                width: page.innerWidth
                device: uds
                deviceName: "Board C UDS Diagnostic Tester"
            }

            MetricCard {
                x: page.margin
                y: 152
                width: page.thirdW
                height: 124
                title: "Last Command"
                value: uds.lastCommand
                detail: uds.lastDid
                accent: "#3ba7ff"
            }
            MetricCard {
                x: page.margin + page.thirdW + page.gap
                y: 152
                width: page.thirdW
                height: 124
                title: "Diagnostic Result"
                value: uds.lastPositive ? "POSITIVE" : "WAITING"
                detail: uds.lastPositive ? "0x62 / OK response" : "send a DID request"
                accent: uds.lastPositive ? "#28c76f" : "#ffb020"
            }
            MetricCard {
                x: page.margin + (page.thirdW + page.gap) * 2
                y: 152
                width: page.thirdW
                height: 124
                title: "Gateway Health"
                value: gateway.warning ? "DTC?" : "OK"
                detail: "Gateway warning " + (gateway.warning ? "active" : "clear")
                accent: gateway.warning ? "#ea5455" : "#28c76f"
            }

            Panel {
                x: page.margin
                y: 292
                width: page.halfW
                height: 360
                title: "ReadDataByIdentifier"
                subtitle: "Board C CLI buttons mapped to project DID table"

                UiButton { x: 0; y: 0; width: (parent.width - 12) / 2; text: "Read VIN 0xF190"; onClicked: uds.readVin() }
                UiButton { x: (parent.width + 12) / 2; y: 0; width: (parent.width - 12) / 2; text: "Read RPM 0xF40C"; onClicked: uds.readRpm() }
                UiButton { x: 0; y: 52; width: (parent.width - 12) / 2; text: "Read Speed 0xF40D"; onClicked: uds.readSpeed() }
                UiButton { x: (parent.width + 12) / 2; y: 52; width: (parent.width - 12) / 2; text: "Read Coolant 0xF40E"; onClicked: uds.readTemp() }
                UiButton { x: 0; y: 104; width: (parent.width - 12) / 2; text: "Read ADAS 0xF410"; onClicked: uds.readAdas() }
                UiButton { x: (parent.width + 12) / 2; y: 104; width: (parent.width - 12) / 2; text: "Read Front 0xF411"; onClicked: uds.readFront() }
                UiButton { x: 0; y: 156; width: (parent.width - 12) / 2; text: "Read Rear 0xF412"; onClicked: uds.readRear() }
                UiButton { x: (parent.width + 12) / 2; y: 156; width: (parent.width - 12) / 2; text: "Read Fault 0xF413"; onClicked: uds.readFault() }
                UiButton { x: 0; y: 218; width: parent.width; height: 44; text: "Clear DTC · SID 0x14"; danger: true; onClicked: uds.clearDtc() }
            }

            Panel {
                x: page.margin + page.halfW + page.gap
                y: 292
                width: page.halfW
                height: 360
                title: "Diagnostic Response"
                subtitle: "Latest CLI response from Board C"

                StatusPill { x: 0; y: 0; width: 180; height: 38; label: "Response"; active: uds.lastPositive; activeText: "POS"; inactiveText: "WAIT" }
                Text { x: 196; y: 9; width: parent.width - 196; text: uds.lastDid; color: "#3ba7ff"; font.pixelSize: 14; font.weight: Font.Bold; elide: Text.ElideRight }
                UiButton {
                    x: parent.width - 96; y: 0; width: 96; height: 38
                    text: "Clear"
                    onClicked: uds.clearResponseLog()
                }

                Rectangle { x: 0; y: 56; width: parent.width; height: parent.height - 56; radius: 8; color: "#0d1115"; border.color: "#2a343f"; border.width: 1 }
                ScrollView {
                    x: 12; y: 68; width: parent.width - 24; height: parent.height - 80
                    clip: true

                    TextArea {
                        id: diagnosticLog
                        text: uds.responseLog
                        color: "#e8edf2"
                        selectedTextColor: "#101418"
                        selectionColor: "#3ba7ff"
                        font.family: "monospace"
                        font.pixelSize: 12
                        readOnly: true
                        wrapMode: TextArea.Wrap
                        selectByMouse: true
                        background: Rectangle { color: "transparent" }
                        onTextChanged: cursorPosition = length
                    }
                }
            }

            Panel {
                x: page.margin
                y: 668
                width: page.innerWidth
                height: 210
                title: "Raw UDS CLI"
                subtitle: "Send direct commands to Board C"

                TextField {
                    id: commandField
                    x: 0; y: 0; width: parent.width - 112; height: 40
                    placeholderText: "read rpm, read speed, read adas, clear dtc ..."
                    color: "#e8edf2"
                    placeholderTextColor: "#8a96a3"
                    background: Rectangle { radius: 8; color: "#0d1115"; border.color: "#2a343f"; border.width: 1 }
                    onAccepted: { uds.sendRaw(text); text = "" }
                }
                UiButton { x: parent.width - 100; y: 0; width: 100; text: "Send"; onClicked: { uds.sendRaw(commandField.text); commandField.text = "" } }
                Text {
                    x: 0; y: 62; width: parent.width
                    text: "Tip: use this tab as a diagnostic tester. The gateway remains the UDS server/diagnostic target for the implemented DID responses."
                    color: "#8a96a3"
                    font.pixelSize: 13
                    wrapMode: Text.WordWrap
                }
            }
        }
    }
}
