import QtQuick
import QtQuick.Controls
import Focusboard
import "components"

ApplicationWindow {
    id: root
    required property DocumentStore documents
    required property SettingsManager settings
    required property CommandController commands
    required property LayerShellWindowController layerController
    visible: false // C++ configures the surface before the first map.
    title: "Focusboard"
    flags: Qt.FramelessWindowHint
    color: "transparent"
    property bool dark: root.settings.theme === "dark" || (root.settings.theme === "system" && root.settings.systemDark)
    property color ink: dark ? "#f0f1f4" : "#252831"
    property string editing: ""
    property bool editLoading: false
    palette.text: ink
    palette.windowText: ink
    palette.base: dark ? "#232630" : "#f6f6f8"
    palette.window: dark ? "#232630" : "#f6f6f8"
    palette.buttonText: ink
    palette.highlight: "#647db7"
    function finishEditing() {
        root.documents.flush()
        editing = ""
        editor.focus = false
        content.forceActiveFocus()
    }
    function openCommands() { palettePopup.open() }
    onClosing: function(event) { event.accepted = false; root.commands.execute(":hide") }
    onVisibleChanged: if (!visible) root.documents.flush()
    Connections {
        target: root.commands
        function onEditRequested(document) {
            root.editLoading = true
            root.editing = document
            editor.text = document === "goals" ? root.documents.goals : root.documents.notes
            root.editLoading = false
            Qt.callLater(function() { editor.forceActiveFocus() })
        }
    }
    Connections {
        target: root.settings
        function onCompactChanged() { root.finishEditing() }
    }
    Connections {
        target: root.documents
        function onGoalsChanged() { if(root.editing === "goals" && editor.text !== root.documents.goals) editor.text = root.documents.goals }
        function onNotesChanged() { if(root.editing === "notes" && editor.text !== root.documents.notes) editor.text = root.documents.notes }
    }
    Rectangle {
        anchors.fill: parent
        radius: 18
        color: root.dark ? "#ed20232b" : "#eef8f8fa"
        border.width: 1
        border.color: root.dark ? "#18ffffff" : "#14000000"
    }
    HoverHandler { id: hover }
    FocusScope {
        id: content
        anchors.fill: parent
        focus: true
        TapHandler { enabled: root.editing === ""; onTapped: content.forceActiveFocus() }
        Keys.onPressed: function(event) {
            if ((event.text === ":" || event.key === Qt.Key_Colon) && root.editing === "") { root.openCommands(); event.accepted = true }
        }
        Flickable {
            id: previewScroll
            visible: root.editing === "" && !root.settings.compact
            anchors.fill: parent
            anchors.margins: 22
            contentWidth: width
            contentHeight: previewColumn.height
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
            Column {
                id: previewColumn
                width: previewScroll.width
                spacing: root.documents.goals.length && root.documents.notes.length ? 18 : 0
                MarkdownPreview { width: parent.width; height: implicitHeight; source: root.documents.goals; visible: source.length > 0; fontSize: root.settings.zoom; foreground: root.ink }
                MarkdownPreview { width: parent.width; height: implicitHeight; source: root.documents.notes; visible: source.length > 0; fontSize: root.settings.zoom; foreground: root.ink }
                Text {
                    width: parent.width
                    visible: !root.documents.goals.length && !root.documents.notes.length
                    text: "A quiet place for your thoughts.\n\nCtrl+K → :edit goals or :edit notes"
                    color: root.ink
                    opacity: 0.65
                    font.pixelSize: root.settings.zoom
                    wrapMode: Text.Wrap
                }
            }
        }
        MarkdownPreview {
            anchors.fill: parent; anchors.margins: 22
            visible: root.settings.compact
            source: (root.documents.goals || root.documents.notes).split("\n").filter(function(line) { return line.trim().length > 0 })[0] || "Ctrl+K to begin"
            clip: true
            fontSize: root.settings.zoom
            foreground: root.ink
        }
        ScrollView {
            id: editorScroll
            anchors.fill: parent; anchors.margins: 18
            visible: root.editing !== ""
            clip: true
            TextArea {
                id: editor
                objectName: "markdownEditor"
                textFormat: TextEdit.PlainText
                wrapMode: TextEdit.Wrap
                selectByMouse: true
                color: root.ink
                font.pixelSize: root.settings.zoom
                font.family: "monospace"
                background: null
                onTextChanged: if (root.editing !== "" && !root.editLoading) root.documents.setDocument(root.editing, text)
                Keys.onPressed: function(event) {
                    if (event.key === Qt.Key_Escape || ((event.key === Qt.Key_Return || event.key === Qt.Key_Enter) && (event.modifiers & Qt.ControlModifier))) {
                        root.finishEditing(); event.accepted = true
                    } else if (event.key === Qt.Key_Y && (event.modifiers & Qt.ControlModifier)) { editor.redo(); event.accepted = true }
                }
            }
        }
    }
    // Hover-only grip. Pointer deltas use screen coordinates, never QWindow::move().
    Rectangle {
        anchors.top: parent.top; anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 5
        width: 38; height: 10; radius: 5
        color: root.ink; opacity: hover.hovered || grip.pressed ? 0.25 : 0
        MouseArea {
            id: grip; anchors.fill: parent; cursorShape: Qt.SizeAllCursor
            onPressed: function(mouse) { const p=mapToGlobal(mouse.x,mouse.y); root.layerController.beginDrag(p.x,p.y) }
            onPositionChanged: function(mouse) { if(pressed) { const p=mapToGlobal(mouse.x,mouse.y); root.layerController.dragTo(p.x,p.y) } }
        }
    }
    ResizeHandle {
        anchors.right: parent.right; anchors.bottom: parent.bottom
        visible: !root.settings.compact
        opacity: hover.hovered || active ? 0.4 : 0
        ink: root.ink
        onResize: function(dx,dy) { root.layerController.resizeBy(dx,dy) }
    }
    CommandPalette {
        id: palettePopup
        objectName: "commandPalette"
        commands: root.commands
        dark: root.dark
        ink: root.ink
        onVisibleChanged: root.layerController.setPaletteOpen(visible)
        parent: Overlay.overlay
        width: root.width - 32
        x: 16; y: 16
        onClosed: if (root.editing !== "") editor.forceActiveFocus(); else content.forceActiveFocus()
    }
    Text {
        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
        anchors.margins: 16
        visible: root.documents.error.length > 0
        text: root.documents.error
        color: root.dark ? "#ffb4a8" : "#992d21"
        font.pixelSize: 12
        wrapMode: Text.Wrap
    }
    Shortcut { sequence: "Ctrl+K"; context: Qt.WindowShortcut; onActivated: root.openCommands() }
    Shortcut { sequence: "Ctrl++"; context: Qt.WindowShortcut; onActivated: root.commands.execute(":zoom +") }
    Shortcut { sequence: "Ctrl+="; context: Qt.WindowShortcut; onActivated: root.commands.execute(":zoom +") }
    Shortcut { sequence: "Ctrl+-"; context: Qt.WindowShortcut; onActivated: root.commands.execute(":zoom -") }
    Shortcut { sequence: "Ctrl+0"; context: Qt.WindowShortcut; onActivated: root.commands.execute(":zoom reset") }
}
