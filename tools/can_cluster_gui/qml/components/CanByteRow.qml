import QtQuick

Row {
    id: root
    property var bytes: ["--", "--", "--", "--", "--", "--", "--", "--"]
    spacing: 7

    Repeater {
        model: root.bytes
        delegate: Rectangle {
            width: 42
            height: 42
            radius: 7
            color: "#0d1115"
            border.color: "#2a343f"
            border.width: 1
            Text {
                anchors.centerIn: parent
                text: modelData
                color: "#e8edf2"
                font.family: "monospace"
                font.pixelSize: 13
                font.weight: Font.Bold
            }
        }
    }
}
