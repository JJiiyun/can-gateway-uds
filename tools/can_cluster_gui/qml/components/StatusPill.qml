import QtQuick

Rectangle {
    id: root
    property string label: ""
    property bool active: false
    property color activeColor: "#28c76f"
    property color inactiveColor: "#8a96a3"
    property string activeText: "ON"
    property string inactiveText: "OFF"

    color: root.active ? Qt.rgba(activeColor.r, activeColor.g, activeColor.b, 0.16) : "#202831"
    border.color: root.active ? root.activeColor : "#2a343f"
    border.width: 1
    radius: 8

    Rectangle {
        x: 12
        y: parent.height / 2 - 5
        width: 10
        height: 10
        radius: 5
        color: root.active ? root.activeColor : root.inactiveColor
    }

    Text {
        x: 30
        y: 0
        width: parent.width - 74
        height: parent.height
        text: root.label
        color: "#e8edf2"
        font.pixelSize: 13
        font.weight: Font.DemiBold
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    Text {
        x: parent.width - 42
        y: 0
        width: 34
        height: parent.height
        text: root.active ? root.activeText : root.inactiveText
        color: root.active ? root.activeColor : root.inactiveColor
        font.pixelSize: 11
        font.weight: Font.Bold
        horizontalAlignment: Text.AlignRight
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
}
