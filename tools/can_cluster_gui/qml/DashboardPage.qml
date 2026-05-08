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
            readonly property int thirdW: (innerWidth - gap * 2) / 3

            MetricCard {
                x: page.margin
                y: 18
                width: page.cardW
                height: 124
                title: "CAN1 Powertrain / Body"
                value: "RX " + gateway.can1Rx
                detail: "TX " + gateway.can1Tx + " · Engine + BCM + Safety"
                accent: "#28c76f"
            }
            MetricCard {
                x: page.margin + (page.cardW + page.gap)
                y: 18
                width: page.cardW
                height: 124
                title: "CAN2 Cluster / Diagnostic"
                value: "TX " + gateway.can2Tx
                detail: "RX " + gateway.can2Rx + " · VW cluster + UDS"
                accent: "#3ba7ff"
            }
            MetricCard {
                x: page.margin + (page.cardW + page.gap) * 2
                y: 18
                width: page.cardW
                height: 124
                title: "Gateway Health"
                value: gateway.warning ? "WARNING" : "NORMAL"
                detail: "busy " + gateway.busy + " · errors " + gateway.errors
                accent: gateway.warning ? "#ffb020" : "#28c76f"
            }
            MetricCard {
                x: page.margin + (page.cardW + page.gap) * 3
                y: 18
                width: page.cardW
                height: 124
                title: "UDS Tester"
                value: uds.lastPositive ? "POSITIVE" : "READY"
                detail: uds.lastDid + " · " + uds.lastCommand
                accent: uds.lastPositive ? "#28c76f" : "#3ba7ff"
            }

            GaugeCard {
                x: page.margin
                y: 158
                width: page.thirdW
                height: 316
                title: "Board A Engine RPM"
                value: gateway.rpm > 0 ? gateway.rpm : engine.rpm
                maxValue: 7000
                unit: "rpm"
                detail: "Gateway decoded 0x100 / Cluster 0x280"
                accent: "#3ba7ff"
            }

            GaugeCard {
                x: page.margin + page.thirdW + page.gap
                y: 158
                width: page.thirdW
                height: 316
                title: "Vehicle Speed"
                value: gateway.speed > 0 ? gateway.speed : engine.speed
                maxValue: 240
                unit: "km/h"
                detail: "Cluster output 0x1A0 Bremse_1"
                accent: "#28c76f"
            }

            Panel {
                x: page.margin + (page.thirdW + page.gap) * 2
                y: 158
                width: page.thirdW
                height: 316
                title: "Cluster Output"
                subtitle: "CAN2 messages currently observed by gateway"

                StatusPill { x: 0; y: 0; width: parent.width; height: 38; label: "0x280 Motor_1 RPM"; active: gateway.clusterRpmActive; activeText: "LIVE"; inactiveText: "IDLE" }
                StatusPill { x: 0; y: 48; width: parent.width; height: 38; label: "0x1A0 Bremse_1 Speed"; active: gateway.clusterSpeedActive; activeText: "LIVE"; inactiveText: "IDLE" }
                StatusPill { x: 0; y: 96; width: parent.width; height: 38; label: "0x390 Body Forward"; active: gateway.clusterBodyActive; activeText: "LIVE"; inactiveText: "IDLE" }
                StatusPill { x: 0; y: 144; width: parent.width; height: 38; label: "0x481 Engine Warning"; active: gateway.warning; activeColor: "#ffb020"; activeText: "WARN"; inactiveText: "OK" }

                Text {
                    x: 0
                    y: 196
                    width: parent.width
                    text: "Latest: " + gateway.latestFrameBus + " " + gateway.latestFrameDir + " " + gateway.latestFrameId
                    color: "#8a96a3"
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }
            }

            Panel {
                x: page.margin
                y: 492
                width: page.halfW
                height: 300
                title: "Body / BCM State"
                subtitle: "Gateway-observed 0x390 plus direct Board D controller state"

                Rectangle {
                    x: 0
                    y: 0
                    width: parent.width
                    height: 156
                    radius: 10
                    color: "#0d1115"
                    border.color: "#2a343f"
                    border.width: 1

                    Rectangle { x: parent.width / 2 - 125; y: 34; width: 250; height: 86; radius: 26; color: "#202831"; border.color: "#3ba7ff"; border.width: 1 }
                    Rectangle { x: parent.width / 2 - 86; y: 18; width: 172; height: 48; radius: 18; color: "#181f26"; border.color: "#2a343f"; border.width: 1 }
                    Rectangle { x: parent.width / 2 - 106; y: 108; width: 48; height: 16; radius: 8; color: "#101418"; border.color: "#2a343f" }
                    Rectangle { x: parent.width / 2 + 58; y: 108; width: 48; height: 16; radius: 8; color: "#101418"; border.color: "#2a343f" }

                    StatusPill { x: 16; y: 18; width: 142; height: 34; label: "FL Door"; active: gateway.doorFl || body.doorFl }
                    StatusPill { x: parent.width - 158; y: 18; width: 142; height: 34; label: "FR Door"; active: gateway.doorFr || body.doorFr }
                    StatusPill { x: 16; y: 64; width: 142; height: 34; label: "RL Door"; active: gateway.doorRl || body.doorRl }
                    StatusPill { x: parent.width - 158; y: 64; width: 142; height: 34; label: "RR Door"; active: gateway.doorRr || body.doorRr }
                    StatusPill { x: 16; y: 110; width: 142; height: 34; label: "Left Turn"; active: gateway.turnLeft || body.turnLeft; activeColor: "#ffb020" }
                    StatusPill { x: parent.width - 158; y: 110; width: 142; height: 34; label: "Right Turn"; active: gateway.turnRight || body.turnRight; activeColor: "#ffb020" }
                }

                StatusPill { x: 0; y: 172; width: (parent.width - 12) / 2; height: 36; label: "High Beam"; active: gateway.highBeam || body.highBeam; activeColor: "#3ba7ff" }
                StatusPill { x: (parent.width + 12) / 2; y: 172; width: (parent.width - 12) / 2; height: 36; label: "Fog Lamp"; active: gateway.fogLamp || body.fogLamp; activeColor: "#3ba7ff" }
            }

            Panel {
                x: page.margin + page.halfW + page.gap
                y: 492
                width: page.halfW
                height: 300
                title: "Latest CAN Frame"
                subtitle: "Gateway UART decode stream"

                Text { x: 0; y: 2; width: 180; text: gateway.latestFrameBus + " " + gateway.latestFrameDir + " " + gateway.latestFrameId; color: "#e8edf2"; font.pixelSize: 22; font.weight: Font.Bold; elide: Text.ElideRight }
                Text { x: 196; y: 8; width: 80; text: "DLC " + gateway.latestFrameDlc; color: "#8a96a3"; font.pixelSize: 13; font.weight: Font.DemiBold }
                CanByteRow { x: 0; y: 54; bytes: gateway.latestFrameBytes }
                Rectangle { x: 0; y: 112; width: parent.width; height: 1; color: "#2a343f" }
                Text { x: 0; y: 130; width: parent.width; text: "Decoded"; color: "#8a96a3"; font.pixelSize: 12; font.weight: Font.DemiBold }
                Text { x: 0; y: 154; width: parent.width; height: 24; text: gateway.latestFrameDecoded; color: "#e8edf2"; font.pixelSize: 15; font.weight: Font.DemiBold; elide: Text.ElideRight }
                Text { x: 0; y: 186; width: parent.width; text: "raw: " + gateway.latestFrameRaw; color: "#8a96a3"; font.family: "monospace"; font.pixelSize: 12; elide: Text.ElideRight }
            }

            Panel {
                x: page.margin
                y: 808
                width: page.innerWidth
                height: 240
                title: "Unified Activity Stream"
                subtitle: "Last serial / CAN / diagnostic messages"

                ListView {
                    id: logList
                    anchors.fill: parent
                    clip: true
                    model: appLog
                    spacing: 4
                    onCountChanged: positionViewAtEnd()
                    delegate: Rectangle {
                        width: logList.width
                        height: 28
                        radius: 6
                        color: index % 2 === 0 ? "#0d1115" : "#151b21"
                        Text { x: 10; y: 0; width: 82; height: parent.height; text: time; color: "#8a96a3"; font.family: "monospace"; font.pixelSize: 11; verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight }
                        Text { x: 104; y: 0; width: 72; height: parent.height; text: source; color: "#3ba7ff"; font.pixelSize: 11; font.weight: Font.Bold; verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight }
                        Text { x: 184; y: 0; width: 56; height: parent.height; text: direction; color: "#8a96a3"; font.family: "monospace"; font.pixelSize: 11; verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight }
                        Text { x: 248; y: 0; width: parent.width - 258; height: parent.height; text: model.text + (decoded.length > 0 ? "  ·  " + decoded : ""); color: "#e8edf2"; font.family: "monospace"; font.pixelSize: 11; verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight }
                    }
                }
            }
        }
    }
}
