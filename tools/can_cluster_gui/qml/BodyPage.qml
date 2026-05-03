import QtQuick
import QtQuick.Controls
import "components"

Rectangle {
    id: root
    color: "#101418"

    component BodyToggle: Rectangle {
        id: toggle
        property string label: ""
        property bool checked: false
        property color accent: "#28c76f"
        signal toggled(bool checked)
        width: 180
        height: 58
        radius: 10
        color: checked ? Qt.rgba(accent.r, accent.g, accent.b, 0.16) : "#202831"
        border.color: checked ? accent : "#2a343f"
        border.width: 1
        Text {
            x: 16
            y: 0
            width: parent.width - 70
            height: parent.height
            text: label
            color: "#e8edf2"
            font.pixelSize: 14
            font.weight: Font.DemiBold
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        Rectangle {
            x: parent.width - 54
            y: parent.height / 2 - 13
            width: 42
            height: 26
            radius: 13
            color: checked ? accent : "#0d1115"
            border.color: checked ? accent : "#2a343f"
            Rectangle {
                x: checked ? 20 : 2
                y: 2
                width: 22
                height: 22
                radius: 11
                color: "#e8edf2"
                Behavior on x { NumberAnimation { duration: 120 } }
            }
        }
        MouseArea {
            anchors.fill: parent
            onClicked: toggle.toggled(!toggle.checked)
        }
    }

    Flickable {
        id: flick
        anchors.fill: parent
        contentWidth: Math.max(width, 1080)
        contentHeight: 1060
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
                device: body
                deviceName: "Board D Body / BCM ECU"
            }

            Panel {
                x: page.margin
                y: 152
                width: page.halfW
                height: 568
                title: "Body Control"
                subtitle: "UART command panel for Board D firmware extension"

                Text { x: 0; y: 0; text: "Control Mode"; color: "#8a96a3"; font.pixelSize: 13; font.weight: Font.DemiBold }
                UiButton { x: 0; y: 28; width: 112; text: "GPIO"; checkable: true; checked: body.mode === "gpio"; onClicked: body.setMode("gpio") }
                UiButton { x: 124; y: 28; width: 112; text: "UART"; checkable: true; checked: body.mode === "uart"; onClicked: body.setMode("uart") }
                StatusPill { x: parent.width - 170; y: 28; width: 170; height: 38; label: "Ignition"; active: body.ignition || gateway.ignition; activeText: "ON"; inactiveText: "OFF" }
                MouseArea { x: parent.width - 170; y: 28; width: 170; height: 38; onClicked: body.setIgnition(!body.ignition) }

                Rectangle { x: 0; y: 82; width: parent.width; height: 1; color: "#2a343f" }
                Text { x: 0; y: 104; text: "Doors"; color: "#e8edf2"; font.pixelSize: 16; font.weight: Font.Bold }
                BodyToggle { x: 0; y: 136; width: (parent.width - 12) / 2; label: "Front Left Door"; checked: body.doorFl; onToggled: body.setDoorFl(checked) }
                BodyToggle { x: (parent.width + 12) / 2; y: 136; width: (parent.width - 12) / 2; label: "Front Right Door"; checked: body.doorFr; onToggled: body.setDoorFr(checked) }
                BodyToggle { x: 0; y: 204; width: (parent.width - 12) / 2; label: "Rear Left Door"; checked: body.doorRl; onToggled: body.setDoorRl(checked) }
                BodyToggle { x: (parent.width + 12) / 2; y: 204; width: (parent.width - 12) / 2; label: "Rear Right Door"; checked: body.doorRr; onToggled: body.setDoorRr(checked) }
                UiButton { x: 0; y: 278; width: 130; text: "All Doors Open"; warning: true; onClicked: body.allDoors(true) }
                UiButton { x: 142; y: 278; width: 138; text: "All Doors Closed"; onClicked: body.allDoors(false) }

                Rectangle { x: 0; y: 334; width: parent.width; height: 1; color: "#2a343f" }
                Text { x: 0; y: 356; text: "Lighting / Signals"; color: "#e8edf2"; font.pixelSize: 16; font.weight: Font.Bold }
                BodyToggle { x: 0; y: 388; width: (parent.width - 12) / 2; label: "Left Turn"; checked: body.turnLeft; accent: "#ffb020"; onToggled: body.setTurnLeft(checked) }
                BodyToggle { x: (parent.width + 12) / 2; y: 388; width: (parent.width - 12) / 2; label: "Right Turn"; checked: body.turnRight; accent: "#ffb020"; onToggled: body.setTurnRight(checked) }
            }

            Panel {
                x: page.margin + page.halfW + page.gap
                y: 152
                width: page.halfW
                height: 568
                title: "Vehicle Body Preview"
                subtitle: "Local Body state + Gateway-observed 0x390"

                Rectangle {
                    x: 0
                    y: 0
                    width: parent.width
                    height: 270
                    radius: 12
                    color: "#0d1115"
                    border.color: "#2a343f"
                    border.width: 1

                    Rectangle { x: parent.width / 2 - 165; y: 82; width: 330; height: 112; radius: 34; color: "#202831"; border.color: "#3ba7ff"; border.width: 2 }
                    Rectangle { x: parent.width / 2 - 108; y: 48; width: 216; height: 70; radius: 22; color: "#181f26"; border.color: "#2a343f"; border.width: 1 }
                    Rectangle { x: parent.width / 2 - 136; y: 184; width: 58; height: 22; radius: 11; color: "#101418"; border.color: "#2a343f" }
                    Rectangle { x: parent.width / 2 + 78; y: 184; width: 58; height: 22; radius: 11; color: "#101418"; border.color: "#2a343f" }

                    Rectangle { x: parent.width / 2 - 172; y: 88; width: 12; height: 44; radius: 6; color: (body.turnLeft || gateway.turnLeft) ? "#ffb020" : "#3a301d" }
                    Rectangle { x: parent.width / 2 + 160; y: 88; width: 12; height: 44; radius: 6; color: (body.turnRight || gateway.turnRight) ? "#ffb020" : "#3a301d" }

                    StatusPill { x: 18; y: 18; width: 154; height: 34; label: "FL"; active: body.doorFl || gateway.doorFl }
                    StatusPill { x: parent.width - 172; y: 18; width: 154; height: 34; label: "FR"; active: body.doorFr || gateway.doorFr }
                    StatusPill { x: 18; y: 218; width: 154; height: 34; label: "RL"; active: body.doorRl || gateway.doorRl }
                    StatusPill { x: parent.width - 172; y: 218; width: 154; height: 34; label: "RR"; active: body.doorRr || gateway.doorRr }
                }

                BodyToggle { x: 0; y: 292; width: (parent.width - 12) / 2; label: "High Beam"; checked: body.highBeam; accent: "#3ba7ff"; onToggled: body.setHighBeam(checked) }
                BodyToggle { x: (parent.width + 12) / 2; y: 292; width: (parent.width - 12) / 2; label: "Fog Lamp"; checked: body.fogLamp; accent: "#3ba7ff"; onToggled: body.setFogLamp(checked) }

                UiButton { x: 0; y: 370; width: 120; text: "Send State"; success: true; onClicked: body.sendState() }
                UiButton { x: 132; y: 370; width: 92; text: "Status"; onClicked: body.status() }
                UiButton { x: 236; y: 370; width: 132; text: "Monitor On"; onClicked: body.monitorOn(200) }
                UiButton { x: 380; y: 370; width: 110; text: "Monitor Off"; onClicked: body.monitorOff() }

                Text {
                    x: 0
                    y: 428
                    width: parent.width
                    text: "Gateway 0x390: " + gateway.lastBodyRx + "  ·  Body TX: " + body.txCount
                    color: "#8a96a3"
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }
            }

            Panel {
                x: page.margin
                y: 738
                width: page.innerWidth
                height: 250
                title: "Raw Body CLI"
                subtitle: "Use after adding UART CLI support to Board D firmware"

                TextField {
                    id: commandField
                    x: 0; y: 0; width: parent.width - 112; height: 40
                    placeholderText: "body door fl 1, body turn left 1, body mode uart ..."
                    color: "#e8edf2"
                    placeholderTextColor: "#8a96a3"
                    background: Rectangle { radius: 8; color: "#0d1115"; border.color: "#2a343f"; border.width: 1 }
                    onAccepted: { body.sendCommand(text); text = "" }
                }
                UiButton { x: parent.width - 100; y: 0; width: 100; text: "Send"; onClicked: { body.sendCommand(commandField.text); commandField.text = "" } }
                Text {
                    x: 0; y: 60; width: parent.width; height: 60
                    text: "."
                    color: "#8a96a3"
                    font.pixelSize: 13
                    wrapMode: Text.WordWrap
                }
            }
        }
    }
}
