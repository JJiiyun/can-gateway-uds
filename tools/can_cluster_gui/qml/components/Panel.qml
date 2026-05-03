import QtQuick
import QtQuick.Controls

Rectangle {
    id: root
    property string title: ""
    property string subtitle: ""
    default property alias content: contentItem.data

    color: "#181f26"
    border.color: "#2a343f"
    border.width: 1
    radius: 10
    clip: true

    Text {
        id: titleText
        x: 18
        y: 14
        width: parent.width - 36
        text: root.title
        color: "#e8edf2"
        font.pixelSize: 18
        font.weight: Font.Bold
        elide: Text.ElideRight
    }

    Text {
        id: subtitleText
        x: 18
        y: 38
        width: parent.width - 36
        text: root.subtitle
        visible: root.subtitle.length > 0
        color: "#8a96a3"
        font.pixelSize: 12
        elide: Text.ElideRight
    }

    Item {
        id: contentItem
        x: 18
        y: root.subtitle.length > 0 ? 62 : 52
        width: parent.width - 36
        height: parent.height - y - 16
    }
}
