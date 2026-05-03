import QtQuick
import QtQuick.Controls

Rectangle {
    id: root
    property string title: ""
    property string value: ""
    property string detail: ""
    property color accent: "#3ba7ff"
    property bool compact: false

    color: "#181f26"
    border.color: "#2a343f"
    border.width: 1
    radius: 10

    Rectangle {
        x: 0
        y: 0
        width: 4
        height: parent.height
        radius: 2
        color: root.accent
        opacity: 0.95
    }

    Rectangle {
        x: parent.width - 34
        y: 22
        width: 12
        height: 12
        radius: 6
        color: root.accent
        opacity: 0.95
    }

    Text {
        x: 20
        y: root.compact ? 12 : 18
        width: parent.width - 58
        text: root.title
        color: "#8a96a3"
        font.pixelSize: root.compact ? 12 : 14
        font.weight: Font.DemiBold
        elide: Text.ElideRight
    }

    Text {
        x: 20
        y: root.compact ? 33 : 48
        width: parent.width - 40
        height: root.compact ? 28 : 42
        text: root.value
        color: "#e8edf2"
        font.pixelSize: root.compact ? 22 : 32
        font.weight: Font.Bold
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    Text {
        x: 20
        y: root.compact ? 67 : 98
        width: parent.width - 40
        text: root.detail
        color: "#8a96a3"
        font.pixelSize: root.compact ? 12 : 13
        elide: Text.ElideRight
    }
}
