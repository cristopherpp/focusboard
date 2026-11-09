import QtQuick
Item {
    id: handle
    width: 24; height: 24
    property color ink
    readonly property bool active: mouse.pressed
    signal resize(int dx, int dy)
    Text { anchors.centerIn: parent; text: "⌟"; color: handle.ink; font.pixelSize: 20 }
    MouseArea {
        id: mouse
        anchors.fill: parent; cursorShape: Qt.SizeFDiagCursor
        property point last
        onPressed: function(event) { last=mapToGlobal(event.x,event.y) }
        onPositionChanged: function(event) {
            if(pressed) { const p=mapToGlobal(event.x,event.y);handle.resize(p.x-last.x,p.y-last.y);last=p }
        }
    }
}
