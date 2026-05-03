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
        contentHeight: 1040
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

            SerialPanel {
                x: page.margin
                y: 18
                width: page.innerWidth
                device: engine
                deviceName: "Board A Engine ECU"
            }

            GaugeCard {
                x: page.margin
                y: 154
                width: page.halfW
                height: 382
                title: "Engine RPM"
                value: engine.rpm
                maxValue: 7000
                unit: "rpm"
                detail: "mode " + engine.mode + " · tx " + engine.txCount
                accent: "#3ba7ff"
            }

            Panel {
                x: page.margin + page.halfW + page.gap
                y: 154
                width: page.halfW
                height: 382
                title: "Pedal Input"
                subtitle: "Board A UART control"

                Text { x: 0; y: 2; text: "Input Mode"; color: "#8a96a3"; font.pixelSize: 13; font.weight: Font.DemiBold }
                UiButton { x: 0; y: 28; width: 112; text: "ADC"; checkable: true; checked: engine.mode === "adc"; onClicked: engine.setMode("adc") }
                UiButton { x: 124; y: 28; width: 112; text: "UART"; checkable: true; checked: engine.mode === "uart"; onClicked: engine.setMode("uart") }
                StatusPill { x: parent.width - 170; y: 28; width: 170; height: 38; label: "Live update"; active: engine.liveUpdate; activeText: "ON"; inactiveText: "OFF" }
                MouseArea { x: parent.width - 170; y: 28; width: 170; height: 38; onClicked: engine.liveUpdate = !engine.liveUpdate }

                Text { x: 0; y: 92; text: "Throttle"; color: "#e8edf2"; font.pixelSize: 15; font.weight: Font.Bold }
                Text { x: parent.width - 70; y: 92; width: 70; text: engine.throttle + "%"; color: "#3ba7ff"; font.pixelSize: 18; font.weight: Font.Bold; horizontalAlignment: Text.AlignRight }
                Slider { x: 0; y: 122; width: parent.width - 92; from: 0; to: 100; stepSize: 1; value: engine.throttle; onMoved: engine.setThrottle(Math.round(value)) }
                SpinBox { x: parent.width - 78; y: 114; width: 78; from: 0; to: 100; value: engine.throttle; editable: true; onValueModified: engine.setThrottle(value) }

                Text { x: 0; y: 164; text: "Brake"; color: "#e8edf2"; font.pixelSize: 15; font.weight: Font.Bold }
                Text { x: parent.width - 70; y: 164; width: 70; text: engine.brake + "%"; color: "#ffb020"; font.pixelSize: 18; font.weight: Font.Bold; horizontalAlignment: Text.AlignRight }
                Slider { x: 0; y: 194; width: parent.width - 92; from: 0; to: 100; stepSize: 1; value: engine.brake; onMoved: engine.setBrake(Math.round(value)) }
                SpinBox { x: parent.width - 78; y: 186; width: 78; from: 0; to: 100; value: engine.brake; editable: true; onValueModified: engine.setBrake(value) }

                UiButton { x: 0; y: 240; width: 128; text: "Apply Pedal"; success: true; onClicked: engine.applyPedal() }
                UiButton { x: 140; y: 240; width: 96; text: "Stop"; danger: true; onClicked: engine.stop() }
                UiButton { x: 248; y: 240; width: 96; text: "Reset"; warning: true; onClicked: engine.resetSimulation() }
            }

            Panel {
                x: page.margin
                y: 552
                width: page.halfW
                height: 208
                title: "Engine Telemetry"
                subtitle: "Direct Board A monitor output"

                MetricCard { x: 0; y: 0; width: (parent.width - 24) / 3; height: 96; compact: true; title: "Speed"; value: engine.speed + " km/h"; detail: "vehicle"; accent: "#28c76f" }
                MetricCard { x: (parent.width - 24) / 3 + 12; y: 0; width: (parent.width - 24) / 3; height: 96; compact: true; title: "Coolant"; value: engine.coolant + " °C"; detail: "temp"; accent: "#ffb020" }
                MetricCard { x: ((parent.width - 24) / 3 + 12) * 2; y: 0; width: (parent.width - 24) / 3; height: 96; compact: true; title: "Tx Count"; value: engine.txCount; detail: "frames"; accent: "#3ba7ff" }
            }

            Panel {
                x: page.margin + page.halfW + page.gap
                y: 552
                width: page.halfW
                height: 232
                title: "Monitor / CLI"
                subtitle: "Engine command console"

                Text { x: 0; y: 0; text: "Monitor interval"; color: "#8a96a3"; font.pixelSize: 13; font.weight: Font.DemiBold }
                SpinBox { id: monitorInterval; x: 0; y: 26; width: 110; from: 50; to: 5000; stepSize: 50; value: 200; editable: true }
                UiButton { x: 124; y: 26; width: 86; text: "On"; success: true; onClicked: engine.monitorOn(monitorInterval.value) }
                UiButton { x: 222; y: 26; width: 86; text: "Off"; onClicked: engine.monitorOff() }
                UiButton { x: 320; y: 26; width: 96; text: "Once"; onClicked: engine.monitorOnce() }

                Rectangle {
                    x: 0; y: 82; width: parent.width; height: 1; color: "#2a343f"
                }
                TextField {
                    id: commandField
                    x: 0; y: 102; width: parent.width - 110; height: 38
                    placeholderText: "status, pedal 50 0, monitor on 200 ..."
                    color: "#e8edf2"
                    placeholderTextColor: "#8a96a3"
                    background: Rectangle { radius: 8; color: "#0d1115"; border.color: "#2a343f"; border.width: 1 }
                    onAccepted: { engine.sendCommand(text); text = "" }
                }
                UiButton { x: parent.width - 98; y: 102; width: 98; text: "Send"; onClicked: { engine.sendCommand(commandField.text); commandField.text = "" } }
            }

            Panel {
                x: page.margin
                y: 802
                width: page.innerWidth
                height: 198
                title: "Gateway Cross-check"
                subtitle: "What Board B currently sees from Board A"
                MetricCard { x: 0; y: 0; width: (parent.width - 36) / 4; height: 86; compact: true; title: "Gateway RPM"; value: gateway.rpm + " rpm"; detail: gateway.lastEngineRx; accent: "#3ba7ff" }
                MetricCard { x: (parent.width - 36) / 4 + 12; y: 0; width: (parent.width - 36) / 4; height: 86; compact: true; title: "Gateway Speed"; value: gateway.speed + " km/h"; detail: "0x100 → 0x1A0"; accent: "#28c76f" }
                MetricCard { x: ((parent.width - 36) / 4 + 12) * 2; y: 0; width: (parent.width - 36) / 4; height: 86; compact: true; title: "Gateway Coolant"; value: gateway.coolant + " °C"; detail: "0x100"; accent: "#ffb020" }
                MetricCard { x: ((parent.width - 36) / 4 + 12) * 3; y: 0; width: (parent.width - 36) / 4; height: 86; compact: true; title: "Ignition"; value: gateway.ignition ? "ON" : "OFF"; detail: "alive " + gateway.alive; accent: gateway.ignition ? "#28c76f" : "#8a96a3" }
            }
        }
    }
}
