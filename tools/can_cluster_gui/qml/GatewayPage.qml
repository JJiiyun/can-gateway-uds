import QtQuick
import QtQuick.Controls
import "components"

Rectangle {
    id: root
    color: "#101418"

    Flickable {
        id: flick
        anchors.fill: parent
        contentWidth: Math.max(width, 1120)
        contentHeight: 1080
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
            readonly property int cardW: (innerWidth - gap * 3) / 4
            readonly property int halfW: (innerWidth - gap) / 2

            SerialPanel {
                x: page.margin
                y: 18
                width: page.innerWidth
                device: gateway
                deviceName: "Board B Central Gateway"
                showReset: true
                onResetRequested: { gateway.resetMonitor(); appLog.clear() }
            }

            MetricCard { x: page.margin; y: 152; width: page.cardW; height: 124; title: "CAN1 RX"; value: gateway.can1Rx; detail: "Powertrain + Body input"; accent: "#28c76f" }
            MetricCard { x: page.margin + page.cardW + page.gap; y: 152; width: page.cardW; height: 124; title: "CAN1 TX"; value: gateway.can1Tx; detail: "Gateway bus output"; accent: "#28c76f" }
            MetricCard { x: page.margin + (page.cardW + page.gap) * 2; y: 152; width: page.cardW; height: 124; title: "CAN2 RX"; value: gateway.can2Rx; detail: "Cluster + diagnostic input"; accent: "#3ba7ff" }
            MetricCard { x: page.margin + (page.cardW + page.gap) * 3; y: 152; width: page.cardW; height: 124; title: "CAN2 TX"; value: gateway.can2Tx; detail: "Cluster output frames"; accent: "#3ba7ff" }

            Panel {
                x: page.margin
                y: 292
                width: page.halfW
                height: 372
                title: "Gateway State"
                subtitle: "Traffic, health, and decoded engine state"

                MetricCard { x: 0; y: 0; width: (parent.width - 24) / 3; height: 92; compact: true; title: "Health"; value: gateway.warning ? "WARN" : "OK"; detail: "errors " + gateway.errors; accent: gateway.warning ? "#ffb020" : "#28c76f" }
                MetricCard { x: (parent.width - 24) / 3 + 12; y: 0; width: (parent.width - 24) / 3; height: 92; compact: true; title: "Busy"; value: gateway.busy; detail: "load"; accent: "#3ba7ff" }
                MetricCard { x: ((parent.width - 24) / 3 + 12) * 2; y: 0; width: (parent.width - 24) / 3; height: 92; compact: true; title: "Alive"; value: gateway.alive; detail: gateway.ignition ? "IGN on" : "IGN off"; accent: gateway.ignition ? "#28c76f" : "#8a96a3" }

                Rectangle { x: 0; y: 112; width: parent.width; height: 1; color: "#2a343f" }
                Text { x: 0; y: 132; text: "Decoded Engine"; color: "#8a96a3"; font.pixelSize: 13; font.weight: Font.DemiBold }
                MetricCard { x: 0; y: 160; width: (parent.width - 24) / 3; height: 92; compact: true; title: "RPM"; value: gateway.rpm; detail: "rpm"; accent: "#3ba7ff" }
                MetricCard { x: (parent.width - 24) / 3 + 12; y: 160; width: (parent.width - 24) / 3; height: 92; compact: true; title: "Speed"; value: gateway.speed; detail: "km/h"; accent: "#28c76f" }
                MetricCard { x: ((parent.width - 24) / 3 + 12) * 2; y: 160; width: (parent.width - 24) / 3; height: 92; compact: true; title: "Coolant"; value: gateway.coolant; detail: "°C"; accent: "#ffb020" }
            }

            Panel {
                x: page.margin + page.halfW + page.gap
                y: 292
                width: page.halfW
                height: 372
                title: "Board D Body Decode"
                subtitle: "Latest 0x390 forwarded to CAN2"

                StatusPill { x: 0; y: 0; width: (parent.width - 12) / 2; height: 38; label: "FL Door"; active: gateway.doorFl }
                StatusPill { x: (parent.width + 12) / 2; y: 0; width: (parent.width - 12) / 2; height: 38; label: "FR Door"; active: gateway.doorFr }
                StatusPill { x: 0; y: 48; width: (parent.width - 12) / 2; height: 38; label: "RL Door"; active: gateway.doorRl }
                StatusPill { x: (parent.width + 12) / 2; y: 48; width: (parent.width - 12) / 2; height: 38; label: "RR Door"; active: gateway.doorRr }
                StatusPill { x: 0; y: 106; width: (parent.width - 12) / 2; height: 38; label: "Left Turn"; active: gateway.turnLeft; activeColor: "#ffb020" }
                StatusPill { x: (parent.width + 12) / 2; y: 106; width: (parent.width - 12) / 2; height: 38; label: "Right Turn"; active: gateway.turnRight; activeColor: "#ffb020" }
                StatusPill { x: 0; y: 154; width: (parent.width - 12) / 2; height: 38; label: "High Beam"; active: gateway.highBeam; activeColor: "#3ba7ff" }
                StatusPill { x: (parent.width + 12) / 2; y: 154; width: (parent.width - 12) / 2; height: 38; label: "Fog Lamp"; active: gateway.fogLamp; activeColor: "#3ba7ff" }
                Rectangle { x: 0; y: 212; width: parent.width; height: 1; color: "#2a343f" }
                Text { x: 0; y: 232; width: parent.width; text: "Last body RX: " + gateway.lastBodyRx; color: "#8a96a3"; font.pixelSize: 13; elide: Text.ElideRight }
            }

            Panel {
                x: page.margin
                y: 680
                width: page.innerWidth
                height: 360
                title: "CAN Log Monitor"

                TextField {
                    id: canIdField
                    x: 0; y: 0; width: 160; height: 38
                    placeholderText: "CAN ID, e.g. 100"
                    color: "#e8edf2"
                    placeholderTextColor: "#8a96a3"
                    background: Rectangle { radius: 8; color: "#0d1115"; border.color: "#2a343f"; border.width: 1 }
                    onAccepted: applyCanFilterButton.clicked()
                }
                UiButton {
                    id: applyCanFilterButton
                    x: 174; y: 0; width: 92; text: "Apply ID"
                    onClicked: {
                        var idText = canIdField.text.trim()
                        if (idText.length === 0) return
                        if (idText.startsWith("0x") || idText.startsWith("0X")) idText = idText.slice(2)
                        gateway.sendCommand("canlog id " + idText)
                        gateway.sendCommand("canlog on")
                    }
                }
                UiButton { x: 278; y: 0; width: 68; text: "All"; onClicked: { gateway.sendCommand("canlog all"); gateway.sendCommand("canlog on") } }
                UiButton { x: 358; y: 0; width: 86; text: "CAN Off"; onClicked: gateway.sendCommand("canlog off") }
                UiButton { x: 456; y: 0; width: 86; text: "GW Log"; success: true; onClicked: gateway.sendCommand("log on") }
                UiButton { x: 554; y: 0; width: 78; text: "GW Off"; onClicked: gateway.sendCommand("log off") }
                UiButton { x: parent.width - 78; y: 0; width: 78; text: "Clear"; onClicked: appLog.clear() }

                Rectangle {
                    x: 0
                    y: 56
                    width: parent.width
                    height: 96
                    radius: 8
                    color: "#202831"
                    border.color: "#2a343f"
                    border.width: 1
                    Text { x: 14; y: 12; width: 160; text: "Latest Frame"; color: "#8a96a3"; font.pixelSize: 12; font.weight: Font.DemiBold }
                    Text { x: 14; y: 38; width: 174; text: gateway.latestFrameBus + " " + gateway.latestFrameDir + " " + gateway.latestFrameId; color: "#e8edf2"; font.pixelSize: 18; font.weight: Font.Bold; elide: Text.ElideRight }
                    Text { x: 14; y: 68; width: 174; text: "DLC " + gateway.latestFrameDlc; color: "#8a96a3"; font.pixelSize: 12 }
                    CanByteRow { x: 208; y: 27; bytes: gateway.latestFrameBytes }
                    Rectangle { x: 604; y: 14; width: 1; height: parent.height - 28; color: "#2a343f" }
                    Text { x: 628; y: 12; width: parent.width - 646; text: "Decoded"; color: "#8a96a3"; font.pixelSize: 12; font.weight: Font.DemiBold; elide: Text.ElideRight }
                    Text { x: 628; y: 38; width: parent.width - 646; text: gateway.latestFrameDecoded; color: "#e8edf2"; font.pixelSize: 15; font.weight: Font.DemiBold; elide: Text.ElideRight }
                    Text { x: 628; y: 68; width: parent.width - 646; text: "raw: " + gateway.latestFrameRaw; color: "#8a96a3"; font.family: "monospace"; font.pixelSize: 12; elide: Text.ElideRight }
                }

                Rectangle {
                    x: 0
                    y: 168
                    width: parent.width
                    height: parent.height - 168
                    radius: 8
                    color: "#0d1115"
                    border.color: "#2a343f"
                    border.width: 1
                    ListView {
                        id: logList
                        anchors.fill: parent
                        anchors.margins: 8
                        clip: true
                        model: appLog
                        spacing: 2
                        onCountChanged: positionViewAtEnd()
                        delegate: Text {
                            width: logList.width
                            height: 22
                            text: time + " | " + source + " | " + direction + " | " + model.text + (decoded.length > 0 ? " | " + decoded : "")
                            color: source === "Gateway" ? "#e8edf2" : "#8a96a3"
                            font.family: "monospace"
                            font.pixelSize: 11
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }
    }
}
