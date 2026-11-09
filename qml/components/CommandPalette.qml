pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Focusboard

Popup {
    id: popup
    required property CommandController commands
    required property bool dark
    required property color ink
    readonly property color secondaryInk: Qt.alpha(ink, 0.72)
    readonly property color accent: dark ? "#a7b5d5" : "#65799f"
    modal: true
    focus: true
    padding: 16
    height: Math.min(implicitHeight, parent.height - 32)
    property int historyIndex: -1
    property string response: ""
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    onOpened: { response = ""; historyIndex = -1; input.text = ":"; input.forceActiveFocus() }
    Overlay.modal: Rectangle { color: popup.dark ? "#14000000" : "#08000000" }
    background: Rectangle {
        radius: 20
        color: popup.dark ? "#fc292c34" : "#fcfaf9f7"
        border.width: 1
        border.color: popup.dark ? "#14ffffff" : "#10000000"
    }
    contentItem: ColumnLayout {
        spacing: 12
        TextField {
            id: input
            objectName: "commandInput"
            Layout.fillWidth: true
            implicitHeight: 46
            leftPadding: 14
            rightPadding: 14
            topPadding: 12
            bottomPadding: 12
            color: popup.ink
            placeholderTextColor: popup.secondaryInk
            selectionColor: Qt.alpha(popup.accent, popup.dark ? 0.35 : 0.22)
            selectedTextColor: popup.ink
            font.pixelSize: 15
            background: Rectangle {
                radius: 12
                color: popup.dark ? "#16ffffff" : "#07000000"
                border.width: 1
                border.color: input.activeFocus ? Qt.alpha(popup.accent, 0.38) : "transparent"
            }
            placeholderText: ":help"
            selectByMouse: true
            onAccepted: { popup.response = popup.commands.execute(text); if (!popup.response.length) popup.close() }
            Keys.onPressed: function(event) {
                if (event.key === Qt.Key_Tab) { text=popup.commands.complete(text); event.accepted=true }
                else if (event.key === Qt.Key_Up) { const next=popup.commands.history(popup.historyIndex+1); if(next.length){popup.historyIndex++;text=next} event.accepted=true }
                else if (event.key === Qt.Key_Down) { popup.historyIndex=Math.max(-1,popup.historyIndex-1);text=popup.historyIndex<0?":":popup.commands.history(popup.historyIndex);event.accepted=true }
            }
        }
        ScrollView {
            id: feedback
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: Math.min(message.implicitHeight, 280)
            clip: true
            Label {
                id: message
                width: feedback.availableWidth
                text: popup.response || "Tab completes · ↑/↓ history · Escape closes"
                textFormat: Text.PlainText
                wrapMode: Text.Wrap
                font.pixelSize: 12
                color: popup.secondaryInk
                lineHeight: 1.3
            }
        }
    }
}
