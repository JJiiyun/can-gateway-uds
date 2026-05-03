import QtQuick

Rectangle {
    id: root
    property string title: ""
    property string unit: ""
    property real value: 0
    property real maxValue: 100
    property color accent: "#3ba7ff"
    property string detail: ""

    color: "#181f26"
    border.color: "#2a343f"
    border.width: 1
    radius: 12

    Canvas {
        id: gauge
        anchors.fill: parent
        anchors.margins: 10
        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            var w = width
            var h = height
            var cx = w / 2
            var cy = h * 0.61
            var r = Math.min(w * 0.38, h * 0.34)
            var start = Math.PI * 0.78
            var end = Math.PI * 2.22
            var ratio = Math.max(0, Math.min(1, root.value / root.maxValue))
            var current = start + (end - start) * ratio

            ctx.lineWidth = 15
            ctx.lineCap = "round"
            ctx.strokeStyle = "#202831"
            ctx.beginPath()
            ctx.arc(cx, cy, r, start, end)
            ctx.stroke()

            ctx.strokeStyle = root.accent
            ctx.beginPath()
            ctx.arc(cx, cy, r, start, current)
            ctx.stroke()

            ctx.lineWidth = 2
            ctx.strokeStyle = "#2a343f"
            for (var i = 0; i <= 8; ++i) {
                var a = start + (end - start) * i / 8
                var x1 = cx + Math.cos(a) * (r - 7)
                var y1 = cy + Math.sin(a) * (r - 7)
                var x2 = cx + Math.cos(a) * (r + 8)
                var y2 = cy + Math.sin(a) * (r + 8)
                ctx.beginPath()
                ctx.moveTo(x1, y1)
                ctx.lineTo(x2, y2)
                ctx.stroke()
            }

            var needle = current
            ctx.strokeStyle = "#e8edf2"
            ctx.lineWidth = 3
            ctx.beginPath()
            ctx.moveTo(cx, cy)
            ctx.lineTo(cx + Math.cos(needle) * (r - 18), cy + Math.sin(needle) * (r - 18))
            ctx.stroke()
            ctx.fillStyle = "#0d1115"
            ctx.beginPath()
            ctx.arc(cx, cy, 8, 0, Math.PI * 2)
            ctx.fill()
            ctx.strokeStyle = root.accent
            ctx.lineWidth = 2
            ctx.stroke()
        }
    }

    onValueChanged: gauge.requestPaint()
    onMaxValueChanged: gauge.requestPaint()
    onAccentChanged: gauge.requestPaint()
    Component.onCompleted: gauge.requestPaint()

    Text {
        x: 18
        y: 15
        width: parent.width - 36
        text: root.title
        color: "#8a96a3"
        font.pixelSize: 14
        font.weight: Font.DemiBold
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        y: parent.height * 0.39
        width: parent.width - 36
        text: Math.round(root.value)
        color: "#e8edf2"
        font.pixelSize: 40
        font.weight: Font.Bold
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        y: parent.height * 0.56
        width: parent.width - 36
        text: root.unit
        color: root.accent
        font.pixelSize: 14
        font.weight: Font.Bold
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
    }

    Text {
        x: 18
        y: parent.height - 35
        width: parent.width - 36
        text: root.detail
        color: "#8a96a3"
        font.pixelSize: 12
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
    }
}
