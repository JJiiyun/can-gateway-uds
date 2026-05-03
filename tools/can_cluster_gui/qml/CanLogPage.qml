import QtQuick
import QtQuick.Controls
import "components"

Rectangle {
    id: root
    color: "#101418"

    Column {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 14

        Rectangle {
            width: parent.width
            height: 68
            radius: 10
            color: "#181f26"
            border.color: "#2a343f"
            border.width: 1

            Text {
                x: 18
                y: 12
                text: "Unified Serial / CAN Log"
                color: "#e8edf2"
                font.pixelSize: 20
                font.weight: Font.Bold
            }
            Text {
                x: 18
                y: 40
                text: appLog.count + " entries · Engine, Body, Gateway, UDS"
                color: "#8a96a3"
                font.pixelSize: 12
            }
            UiButton {
                x: parent.width - 102
                y: 15
                width: 84
                text: "Clear"
                danger: true
                onClicked: appLog.clear()
            }
        }

        Rectangle {
            width: parent.width
            height: parent.height - 82
            radius: 10
            color: "#181f26"
            border.color: "#2a343f"
            border.width: 1

            Rectangle {
                x: 12
                y: 12
                width: parent.width - 24
                height: 34
                radius: 8
                color: "#0d1115"
                border.color: "#2a343f"
                border.width: 1
                Text { x: 12; y: 0; width: 90; height: parent.height; text: "Time"; color: "#8a96a3"; font.pixelSize: 12; font.weight: Font.Bold; verticalAlignment: Text.AlignVCenter }
                Text { x: 116; y: 0; width: 90; height: parent.height; text: "Source"; color: "#8a96a3"; font.pixelSize: 12; font.weight: Font.Bold; verticalAlignment: Text.AlignVCenter }
                Text { x: 220; y: 0; width: 80; height: parent.height; text: "Dir"; color: "#8a96a3"; font.pixelSize: 12; font.weight: Font.Bold; verticalAlignment: Text.AlignVCenter }
                Text { x: 312; y: 0; width: parent.width - 324; height: parent.height; text: "Payload / Decoded"; color: "#8a96a3"; font.pixelSize: 12; font.weight: Font.Bold; verticalAlignment: Text.AlignVCenter }
            }

            ListView {
                id: logList
                x: 12
                y: 54
                width: parent.width - 24
                height: parent.height - 66
                clip: true
                model: appLog
                spacing: 5
                onCountChanged: positionViewAtEnd()

                delegate: Rectangle {
                    width: logList.width
                    height: decoded && decoded.length > 0 ? 56 : 38
                    radius: 8
                    color: index % 2 === 0 ? "#0d1115" : "#151b21"
                    border.color: "#202831"
                    border.width: 1

                    Text { x: 12; y: 0; width: 90; height: parent.height; text: time; color: "#8a96a3"; font.family: "monospace"; font.pixelSize: 11; verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight }
                    Text { x: 116; y: 0; width: 90; height: parent.height; text: source; color: source === "Gateway" ? "#3ba7ff" : source === "Engine" ? "#28c76f" : source === "UDS" ? "#ffb020" : "#e8edf2"; font.pixelSize: 12; font.weight: Font.Bold; verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight }
                    Text { x: 220; y: 0; width: 80; height: parent.height; text: direction; color: "#8a96a3"; font.family: "monospace"; font.pixelSize: 11; verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight }
                    Text { x: 312; y: decoded && decoded.length > 0 ? 8 : 0; width: parent.width - 324; height: decoded && decoded.length > 0 ? 20 : parent.height; text: model.text; color: "#e8edf2"; font.family: "monospace"; font.pixelSize: 12; verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight }
                    Text { x: 312; y: 30; width: parent.width - 324; height: 20; visible: decoded && decoded.length > 0; text: decoded; color: "#8a96a3"; font.family: "monospace"; font.pixelSize: 11; verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight }
                }
            }
        }
    }
}
