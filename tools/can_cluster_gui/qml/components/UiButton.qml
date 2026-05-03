import QtQuick
import QtQuick.Controls

Button {
    id: root
    property color accent: "#3ba7ff"
    property bool danger: false
    property bool success: false
    property bool warning: false

    implicitHeight: 38
    implicitWidth: 112
    font.pixelSize: 13
    font.weight: Font.DemiBold

    contentItem: Text {
        text: root.text
        color: root.enabled ? "#e8edf2" : "#697480"
        font: root.font
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: 8
        color: {
            if (!root.enabled) return "#171d23"
            if (root.down || root.checked) {
                if (root.danger) return "#5e2528"
                if (root.warning) return "#5a4219"
                if (root.success) return "#20452e"
                return "#1f4262"
            }
            if (root.danger) return "#3a2225"
            if (root.warning) return "#3a301d"
            if (root.success) return "#203b2d"
            return "#202831"
        }
        border.width: 1
        border.color: {
            if (!root.enabled) return "#242b33"
            if (root.danger) return "#ea5455"
            if (root.warning) return "#ffb020"
            if (root.success) return "#28c76f"
            return root.down || root.checked ? root.accent : "#2a343f"
        }
    }
}
