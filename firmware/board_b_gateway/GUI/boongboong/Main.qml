import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: root
    width: 1280
    height: 820
    minimumWidth: 1060
    minimumHeight: 700
    visible: true
    title: qsTr("Board B Gateway Monitor")
    color: "#101418"

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

    QtObject {
        id: gateway
        property bool connected: serialBridge.connected
        property int can1Rx: serialBridge.can1Rx
        property int can1Tx: serialBridge.can1Tx
        property int can2Rx: serialBridge.can2Rx
        property int can2Tx: serialBridge.can2Tx
        property int busy: serialBridge.busy
        property int errors: serialBridge.errors
        property bool warning: serialBridge.warning
        property bool engineRpmWarning: serialBridge.engineRpmWarning
        property bool engineCoolantWarning: serialBridge.engineCoolantWarning
        property bool engineGeneralWarning: serialBridge.engineGeneralWarning
        property int routeMatched: serialBridge.routeMatched
        property int routeOk: serialBridge.routeOk
        property int routeFail: serialBridge.routeFail
        property int routeIgnored: serialBridge.routeIgnored
        property int rpm: serialBridge.rpm
        property int speed: serialBridge.speed
        property int coolant: serialBridge.coolant
        property bool ignition: serialBridge.ignition
        property bool doorFl: serialBridge.doorFl
        property bool doorFr: serialBridge.doorFr
        property bool doorRl: serialBridge.doorRl
        property bool doorRr: serialBridge.doorRr
        property bool turnLeft: serialBridge.turnLeft
        property bool turnRight: serialBridge.turnRight
        property bool highBeam: serialBridge.highBeam
        property bool fogLamp: serialBridge.fogLamp
        property bool adasValid: serialBridge.adasValid
        property int adasRisk: serialBridge.adasRisk
        property int adasFault: serialBridge.adasFault
        property int adasDtc: serialBridge.adasDtc
        property int adasFront: serialBridge.adasFront
        property int adasRear: serialBridge.adasRear
        property int adasSpeed: serialBridge.adasSpeed
        property int adasAlive: serialBridge.adasAlive
    }

    ListModel { id: logModel }

    Component.onCompleted: serialBridge.refreshPorts()

    Connections {
        target: serialBridge

        function appendLine(line) {
            if (logModel.count > 300)
                logModel.remove(0)
            logModel.append({ text: line })
            logList.positionViewAtEnd()
        }

        function onGatewayLineReceived(time, line) {
            appendLine(time + " | UART | GW | " + line)
        }

        function onFrameLineReceived(time, bus, dir, frameId, dlc, payload, decoded) {
            appendLine(time + " | " + bus + " | " + dir + " | " + frameId + " | " + dlc + " | " + payload + " | " + decoded)
        }

        function onRawLineReceived(time, line) {
            appendLine(time + " | UART | -- | " + line)
        }
    }

    component MetricCard: Rectangle {
        property string title: ""
        property string value: ""
        property string detail: ""
        property color accent: root.accentColor

        color: root.panelColor
        border.color: root.panelBorder
        border.width: 1
        radius: 8

        Text {
            id: metricTitle
            x: 18
            y: 18
            width: parent.width - 54
            text: title
            color: root.textMuted
            font.pixelSize: 15
            font.weight: Font.DemiBold
            elide: Text.ElideRight
        }

        Rectangle {
            x: parent.width - 28
            y: 24
            width: 10
            height: 10
            radius: 5
            color: accent
        }

        Text {
            x: 18
            y: 48
            width: parent.width - 36
            height: 42
            text: value
            color: root.textPrimary
            font.pixelSize: 32
            font.weight: Font.Bold
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }

        Text {
            x: 18
            y: 98
            width: parent.width - 36
            text: detail
            color: root.textMuted
            font.pixelSize: 14
            elide: Text.ElideRight
        }
    }

    component SmallTile: Rectangle {
        property string label: ""
        property string value: ""

        color: root.panelSoft
        radius: 7

        Text {
            x: 8
            y: 12
            width: parent.width - 16
            text: label
            color: root.textMuted
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 12
            elide: Text.ElideRight
        }

        Text {
            x: 8
            y: 34
            width: parent.width - 16
            text: value
            color: root.textPrimary
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 17
            font.weight: Font.Bold
            elide: Text.ElideRight
        }
    }

    component StatusPill: Rectangle {
        property string label: ""
        property bool active: false
        property color activeColor: root.goodColor
        property color activeBg: "#203b2d"

        color: active ? activeBg : root.panelSoft
        border.color: active ? activeColor : root.panelBorder
        border.width: 1
        radius: 6

        Rectangle {
            x: 10
            y: parent.height / 2 - 4
            width: 8
            height: 8
            radius: 4
            color: active ? activeColor : root.textMuted
        }

        Text {
            x: 26
            y: 0
            width: parent.width - 34
            height: parent.height
            text: label
            color: root.textPrimary
            font.pixelSize: 13
            font.weight: Font.DemiBold
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }

    header: Rectangle {
        height: 104
        color: root.panelDark
        border.color: root.panelBorder
        border.width: 1
        clip: true

        Text {
            id: headerTitle
            x: 22
            y: 0
            width: parent.width - 44
            height: 42
            text: "Board B Gateway Monitor"
            color: root.textPrimary
            font.pixelSize: 21
            font.weight: Font.Bold
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        ComboBox {
            id: portCombo
            x: 22
            y: 52
            width: Math.max(230, Math.min(320, parent.width * 0.26))
            height: 36
            model: serialBridge.portNames
        }

        ComboBox {
            id: baudCombo
            x: portCombo.x + portCombo.width + 16
            y: 52
            width: 116
            height: 36
            model: ["115200", "921600"]
        }

        Button {
            x: baudCombo.x + baudCombo.width + 16
            y: 52
            width: 118
            height: 36
            text: gateway.connected ? "Disconnect" : "Connect"
            onClicked: {
                if (gateway.connected)
                    serialBridge.disconnectPort()
                else
                    serialBridge.connectToPort(portCombo.currentText, parseInt(baudCombo.currentText))
            }
        }

        Button {
            x: baudCombo.x + baudCombo.width + 150
            y: 52
            width: 90
            height: 36
            text: "Refresh"
            onClicked: {
                serialBridge.resetMonitor()
                logModel.clear()
                canIdField.text = ""
            }
        }

        Rectangle {
            id: statusBadge
            x: parent.width - statusText.width - width - 34
            y: 54
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

        Text {
            id: statusText
            x: parent.width - width - 18
            y: 52
            width: 190
            height: 36
            text: serialBridge.statusText
            color: root.textMuted
            font.pixelSize: 12
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }

    Flickable {
        id: flick
        anchors.fill: parent
        anchors.margins: 0
        contentWidth: Math.max(width, 1040)
        contentHeight: page.height
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
        }

        Item {
            id: page
            width: flick.contentWidth
            height: 1070

            readonly property int margin: 18
            readonly property int gap: 14
            readonly property int innerWidth: width - margin * 2
            readonly property int cardW: (innerWidth - gap * 2) / 3
            readonly property int halfW: (innerWidth - gap) / 2

            MetricCard {
                x: page.margin
                y: 18
                width: page.cardW
                height: 132
                title: "CAN1 Bus"
                value: "RX " + gateway.can1Rx
                detail: "TX " + gateway.can1Tx + " / Powertrain + Body"
                accent: root.goodColor
            }

            MetricCard {
                x: page.margin + page.cardW + page.gap
                y: 18
                width: page.cardW
                height: 132
                title: "CAN2 Bus"
                value: "TX " + gateway.can2Tx
                detail: "RX " + gateway.can2Rx + " / Cluster + Diagnostic"
                accent: root.accentColor
            }

            MetricCard {
                x: page.margin + (page.cardW + page.gap) * 2
                y: 18
                width: page.cardW
                height: 132
                title: "Gateway Health"
                value: gateway.warning ? "WARNING" : "NORMAL"
                detail: "busy " + gateway.busy + " / err " + gateway.errors + " / fail " + gateway.routeFail
                accent: gateway.warning ? root.warnColor : root.goodColor
            }

            Rectangle {
                id: routerPanel
                x: page.margin
                y: 164
                width: page.halfW
                height: 420
                color: root.panelColor
                border.color: root.panelBorder
                border.width: 1
                radius: 8

                Text {
                    x: 18
                    y: 18
                    width: parent.width - 36
                    text: "CAN1 Input Boards"
                    color: root.textPrimary
                    font.pixelSize: 20
                    font.weight: Font.Bold
                    elide: Text.ElideRight
                }

                Text {
                    x: 18
                    y: 54
                    width: parent.width - 36
                    text: "Board A Engine / Cluster Source"
                    color: root.textPrimary
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                }

                readonly property int tileW: (width - 78) / 4

                SmallTile { x: 18; y: 86; width: routerPanel.tileW; height: 58; label: "RPM"; value: gateway.rpm }
                SmallTile { x: 32 + routerPanel.tileW; y: 86; width: routerPanel.tileW; height: 58; label: "Speed"; value: gateway.speed + " km/h" }
                SmallTile { x: 46 + routerPanel.tileW * 2; y: 86; width: routerPanel.tileW; height: 58; label: "Coolant"; value: gateway.coolant }
                SmallTile { x: 60 + routerPanel.tileW * 3; y: 86; width: routerPanel.tileW; height: 58; label: "IGN"; value: gateway.ignition ? "ON" : "OFF" }

                StatusPill { x: 18; y: 154; width: (parent.width - 64) / 3; height: 34; label: "RPM Warn"; active: gateway.engineRpmWarning; activeColor: root.badColor; activeBg: "#3a2024" }
                StatusPill { x: 32 + (parent.width - 64) / 3; y: 154; width: (parent.width - 64) / 3; height: 34; label: "Coolant Warn"; active: gateway.engineCoolantWarning; activeColor: root.warnColor; activeBg: "#3b321e" }
                StatusPill { x: 46 + ((parent.width - 64) / 3) * 2; y: 154; width: (parent.width - 64) / 3; height: 34; label: "General Warn"; active: gateway.engineGeneralWarning; activeColor: root.warnColor; activeBg: "#3b321e" }

                Text {
                    x: 18
                    y: 206
                    width: parent.width - 36
                    text: "Board D Body Input"
                    color: root.textPrimary
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                }

                StatusPill { x: 18; y: 238; width: routerPanel.tileW; height: 34; label: "Left"; active: gateway.turnLeft }
                StatusPill { x: 32 + routerPanel.tileW; y: 238; width: routerPanel.tileW; height: 34; label: "Right"; active: gateway.turnRight }
                StatusPill { x: 46 + routerPanel.tileW * 2; y: 238; width: routerPanel.tileW; height: 34; label: "Hazard"; active: gateway.turnLeft && gateway.turnRight }
                StatusPill { x: 60 + routerPanel.tileW * 3; y: 238; width: routerPanel.tileW; height: 34; label: "Seen"; active: serialBridge.lastBodyRx !== "-" }

                Text {
                    x: 18
                    y: 294
                    width: parent.width - 36
                    text: "Board E Safety Input"
                    color: root.textPrimary
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                }

                SmallTile { x: 18; y: 326; width: routerPanel.tileW; height: 58; label: "Risk"; value: gateway.adasRisk }
                SmallTile { x: 32 + routerPanel.tileW; y: 326; width: routerPanel.tileW; height: 58; label: "Front"; value: gateway.adasFront + " cm" }
                SmallTile { x: 46 + routerPanel.tileW * 2; y: 326; width: routerPanel.tileW; height: 58; label: "Rear"; value: gateway.adasRear + " cm" }
                SmallTile { x: 60 + routerPanel.tileW * 3; y: 326; width: routerPanel.tileW; height: 58; label: "Fault"; value: "0x" + gateway.adasFault.toString(16).toUpperCase() }
            }

            Rectangle {
                id: safetyPanel
                x: page.margin + page.halfW + page.gap
                y: 164
                width: page.halfW
                height: 420
                color: root.panelColor
                border.color: root.panelBorder
                border.width: 1
                radius: 8

                Text {
                    x: 18
                    y: 18
                    width: parent.width - 36
                    text: "Board B Gateway / CAN2 Outputs"
                    color: root.textPrimary
                    font.pixelSize: 20
                    font.weight: Font.Bold
                    elide: Text.ElideRight
                }

                readonly property int pillW: (width - 60) / 4

                SmallTile { x: 18; y: 60; width: safetyPanel.pillW; height: 58; label: "Routed"; value: gateway.routeMatched }
                SmallTile { x: 28 + safetyPanel.pillW; y: 60; width: safetyPanel.pillW; height: 58; label: "OK"; value: gateway.routeOk }
                SmallTile { x: 38 + safetyPanel.pillW * 2; y: 60; width: safetyPanel.pillW; height: 58; label: "Fail"; value: gateway.routeFail }
                SmallTile { x: 48 + safetyPanel.pillW * 3; y: 60; width: safetyPanel.pillW; height: 58; label: "Ignored"; value: gateway.routeIgnored }

                StatusPill { x: 18; y: 132; width: safetyPanel.pillW; height: 34; label: "ADAS"; active: gateway.adasValid }
                StatusPill { x: 28 + safetyPanel.pillW; y: 132; width: safetyPanel.pillW; height: 34; label: "Warning"; active: gateway.warning }
                StatusPill { x: 38 + safetyPanel.pillW * 2; y: 132; width: safetyPanel.pillW; height: 34; label: "Fault"; active: gateway.adasFault !== 0 }
                StatusPill { x: 48 + safetyPanel.pillW * 3; y: 132; width: safetyPanel.pillW; height: 34; label: "DTC"; active: gateway.adasDtc !== 0 }

                Rectangle {
                    x: 18
                    y: 184
                    width: parent.width - 36
                    height: 152
                    radius: 8
                    color: root.panelSoft

                    Text {
                        x: 14
                        y: 12
                        width: parent.width - 28
                        text: "Gateway Frame Watch"
                        color: root.textPrimary
                        font.pixelSize: 16
                        font.weight: Font.Bold
                        elide: Text.ElideRight
                    }

                    ListModel {
                        id: can2OutputModel
                        ListElement { frameId: "0x100"; name: "IGN status input"; period: "50 ms"; key: "ign" }
                        ListElement { frameId: "0x280"; name: "RPM Motor_1"; period: "50 ms"; key: "rpm" }
                        ListElement { frameId: "0x1A0"; name: "Speed Bremse_1"; period: "50 ms"; key: "speed" }
                        ListElement { frameId: "0x5A0"; name: "Speed needle"; period: "50 ms"; key: "needle" }
                        ListElement { frameId: "0x288"; name: "Coolant Motor_2"; period: "100 ms"; key: "coolant" }
                        ListElement { frameId: "0x531"; name: "Turn status"; period: "100 ms"; key: "turn" }
                        ListElement { frameId: "0x481"; name: "Engine warning input"; period: "100 ms"; key: "warning" }
                    }

                    ListView {
                        id: can2OutputList
                        x: 14
                        y: 42
                        width: parent.width - 28
                        height: parent.height - 52
                        clip: true
                        model: can2OutputModel
                        boundsBehavior: Flickable.StopAtBounds

                        ScrollBar.vertical: ScrollBar {
                            policy: ScrollBar.AsNeeded
                        }

                        delegate: Item {
                            width: ListView.view.width
                            height: 30

                            readonly property bool active:
                                key === "ign" ? serialBridge.clusterIgnActive :
                                key === "rpm" ? serialBridge.clusterRpmActive :
                                key === "speed" ? serialBridge.clusterSpeedActive :
                                key === "needle" ? serialBridge.clusterSpeedNeedleActive :
                                key === "coolant" ? serialBridge.clusterCoolantActive :
                                key === "turn" ? serialBridge.clusterTurnActive :
                                gateway.warning

                            Text {
                                x: 0
                                y: 0
                                width: 58
                                height: parent.height
                                text: frameId
                                color: root.accentColor
                                font.pixelSize: 14
                                font.weight: Font.Bold
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }

                            Text {
                                x: 68
                                y: 0
                                width: parent.width - 164
                                height: parent.height
                                text: name
                                color: root.textPrimary
                                font.pixelSize: 14
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }

                            Text {
                                x: parent.width - 86
                                y: 0
                                width: 64
                                height: parent.height
                                text: period
                                color: root.textMuted
                                font.pixelSize: 13
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }

                            Rectangle {
                                x: parent.width - 10
                                y: parent.height / 2 - 5
                                width: 10
                                height: 10
                                radius: 5
                                color: active ? root.goodColor : root.textMuted
                            }
                        }
                    }
                }
            }

            Rectangle {
                id: logPanel
                x: page.margin
                y: 598
                width: page.innerWidth
                height: 430
                color: root.panelColor
                border.color: root.panelBorder
                border.width: 1
                radius: 8

                Text {
                    x: 18
                    y: 18
                    width: 96
                    height: 36
                    text: "CAN Log"
                    color: root.textPrimary
                    font.pixelSize: 18
                    font.weight: Font.Bold
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }

                TextField {
                    id: canIdField
                    x: 124
                    y: 18
                    width: 160
                    height: 36
                    placeholderText: "CAN ID, e.g. 100"
                    color: root.textPrimary
                    placeholderTextColor: root.textMuted
                    inputMethodHints: Qt.ImhUppercaseOnly
                    onAccepted: applyCanFilterButton.clicked()
                }

                Button {
                    id: applyCanFilterButton
                    x: 304
                    y: 18
                    width: 92
                    height: 36
                    text: "Apply ID"
                    onClicked: {
                        var idText = canIdField.text.trim()
                        if (idText.length === 0)
                            return
                        if (idText.startsWith("0x") || idText.startsWith("0X"))
                            idText = idText.slice(2)
                        serialBridge.sendCommand("canlog id " + idText)
                        serialBridge.sendCommand("canlog on")
                    }
                }

                Button {
                    x: 416
                    y: 18
                    width: 64
                    height: 36
                    text: "All"
                    onClicked: {
                        serialBridge.sendCommand("canlog all")
                        serialBridge.sendCommand("canlog on")
                    }
                }

                Button {
                    x: 500
                    y: 18
                    width: 84
                    height: 36
                    text: "CAN Off"
                    onClicked: {
                        serialBridge.sendCommand("canlog off")
                        serialBridge.sendCommand("canlog off")
                        serialBridge.sendCommand("canlog off")
                    }
                }

                Button {
                    x: 604
                    y: 18
                    width: 76
                    height: 36
                    text: "GW Log"
                    onClicked: serialBridge.sendCommand("log on")
                }

                Button {
                    x: 700
                    y: 18
                    width: 76
                    height: 36
                    text: "GW Off"
                    onClicked: serialBridge.sendCommand("log off")
                }

                Button {
                    x: parent.width - 92
                    y: 18
                    width: 74
                    height: 36
                    text: "Clear"
                    onClicked: logModel.clear()
                }

                Rectangle {
                    id: latestPanel
                    x: 18
                    y: 70
                    width: parent.width - 36
                    height: 96
                    radius: 6
                    color: root.panelSoft
                    border.color: root.panelBorder

                    Text {
                        x: 14
                        y: 13
                        width: 160
                        text: "Latest Frame"
                        color: root.textMuted
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        elide: Text.ElideRight
                    }

                    Text {
                        x: 14
                        y: 38
                        width: 160
                        text: serialBridge.latestFrameBus + " " + serialBridge.latestFrameDir + " " + serialBridge.latestFrameId
                        color: root.textPrimary
                        font.pixelSize: 18
                        font.weight: Font.Bold
                        elide: Text.ElideRight
                    }

                    Text {
                        x: 14
                        y: 68
                        width: 160
                        text: "DLC " + serialBridge.latestFrameDlc
                        color: root.textMuted
                        font.pixelSize: 12
                        elide: Text.ElideRight
                    }

                    Repeater {
                        model: serialBridge.latestFrameBytes

                        delegate: Rectangle {
                            x: 200 + index * 49
                            y: 27
                            width: 42
                            height: 42
                            radius: 6
                            color: root.panelDark
                            border.color: root.panelBorder

                            Text {
                                anchors.centerIn: parent
                                text: modelData
                                color: root.textPrimary
                                font.family: "Menlo"
                                font.pixelSize: 14
                                font.weight: Font.Bold
                            }
                        }
                    }

                    Rectangle {
                        x: 604
                        y: 14
                        width: 1
                        height: parent.height - 28
                        color: root.panelBorder
                    }

                    Text {
                        x: 630
                        y: 13
                        width: parent.width - 650
                        text: "Decoded"
                        color: root.textMuted
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        elide: Text.ElideRight
                    }

                    Text {
                        x: 630
                        y: 38
                        width: parent.width - 650
                        text: serialBridge.latestFrameDecoded
                        color: root.textPrimary
                        font.pixelSize: 15
                        font.weight: Font.DemiBold
                        elide: Text.ElideRight
                    }

                    Text {
                        x: 630
                        y: 68
                        width: parent.width - 650
                        text: "raw: " + serialBridge.latestFrameRaw
                        color: root.textMuted
                        font.family: "Menlo"
                        font.pixelSize: 12
                        elide: Text.ElideRight
                    }
                }

                Rectangle {
                    x: 18
                    y: 184
                    width: parent.width - 36
                    height: parent.height - 202
                    radius: 6
                    color: root.panelDark
                    border.color: root.panelBorder

                    ListView {
                        id: logList
                        anchors.fill: parent
                        anchors.margins: 8
                        clip: true
                        model: logModel

                        delegate: Text {
                            width: ListView.view.width
                            height: 24
                            text: model.text
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
