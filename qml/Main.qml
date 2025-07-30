import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCore

import org.qaengine.qainspector

Window {
    width: 1200
    height: 800
    visible: true
    title: qsTr("QAInspector")

    property var widths: [200, 120, 120, 100, 50, 50, 50, 50, 50, 50]
    readonly property int totalWidth: widths.reduce((a, b) => a + b, 0)

    readonly property string settingsLocation: StandardPaths.writableLocation(StandardPaths.AppConfigLocation) + "/qainspector.ini"

    minimumWidth: 600
    minimumHeight: 300

    property bool loaded: false
    property string filters: ""

    onClosing: {
        appSettings.width = width
        appSettings.height = height
        appSettings.x = x
        appSettings.y = y
        appSettings.columnsLayout = JSON.stringify(widths)
        appSettings.appWindowSplitViewLeft = leftLayout.SplitView.preferredWidth
        appSettings.sync()

        connectionSettings.address = ipField.text
        connectionSettings.port = portField.text
        connectionSettings.sync()

        filterSettings.value = filters
        filterSettings.sync()
    }

    Component.onCompleted: {
        if (appSettings.width > 0) {
            width = appSettings.width
            height = appSettings.height
            x = appSettings.x
            y = appSettings.y
        }
        if (appSettings.columnsLayout) {
            console.log('layout:', appSettings.columnsLayout)
            widths = JSON.parse(appSettings.columnsLayout)
            treeView.forceLayout()
        }
        if (appSettings.appWindowSplitViewLeft > 0) {
            leftLayout.SplitView.preferredWidth = appSettings.appWindowSplitViewLeft
        }
        if (connectionSettings.address)
            ipField.text = connectionSettings.address
        if (connectionSettings.port)
            portField.text = connectionSettings.port
        if (filterSettings.value)
            filters = filterSettings.value
    }

    Timer {
        running: true
        repeat: false
        interval: 1000
        onTriggered: loaded = true
    }

    Settings {
        id: appSettings
        category: "MainWindow"
        location: settingsLocation
        property int width
        property int height
        property int x
        property int y
        property string columnsLayout
        property int appWindowSplitViewLeft
    }

    Settings {
        id: connectionSettings
        category: "Connection"
        location: settingsLocation
        property string address
        property string port

        Component.onCompleted: {
            console.log("Connection location:", location)
        }
    }

    Settings {
        id: filterSettings
        category: "Filters"
        location: settingsLocation
        property string value
    }

    TextMetrics {
        id: metrics
        font.pixelSize: 14
    }

    function computeMaxWidth(column, parentRow) {
        var maxW = 0
        var startRow = (parentRow === undefined ? 0 : parentRow + 1)
        var rows = treeView.rows

        for (var row = startRow; row < rows; ++row) {
            var idx = treeView.modelIndex(Qt.point(column, row))
            var text = treeView.model.data(idx, Qt.DisplayRole)
            metrics.text = text
            const mwidth = metrics.width + (column ? 0 : treeView.depth(row) * 18 + 38)
            maxW = Math.max(maxW, mwidth)
        }
        return maxW
    }

    Connections {
        target: SocketConnector

        function onConnectedChanged() {
            if (SocketConnector.connected) {
                treeModel.loadDump(SocketConnector.getDumpTree(filters))
                SocketConnector.getGrabWindow()
            }
        }

        function onImageData(b64) {
            screenshot.setFromB64(b64)
        }
    }

    TextEdit {
        id: clipboard
        visible: false
        function copyText(t) {
            text = t
            selectAll()
            cut()
        }
    }

    RowLayout {
        id: topLayout
        width: parent.width
        spacing: 10

        TextField {
            id: ipField

            Layout.leftMargin: 10
            Layout.fillWidth: true
            Layout.minimumWidth: 100
            leftPadding: 5
            rightPadding: 5

            placeholderText: "device ip address"
            text: "127.0.0.1"

            focus: true

            Keys.onReturnPressed: {
                if (text) {
                    portField.forceActiveFocus()
                }
            }
        }

        TextField {
            id: portField

            leftPadding: 5
            rightPadding: 5

            placeholderText: "port"
            text: "8888"

            implicitWidth: 50

            inputMethodHints: Qt.ImhDigitsOnly

            Keys.onReturnPressed: {
                if (!ipField.text) {
                    ipField.forceActiveFocus()
                } else {
                    connectButton.clicked()
                }
            }

        }

        Button {
            id: connectButton
            text: SocketConnector.connected ? "Disconnect" : "Connect"

            onClicked: {
                console.log("do connect")
                SocketConnector.hostname = ipField.text
                SocketConnector.port = portField.text
                SocketConnector.connected = !SocketConnector.connected
            }
        }

        Button {
            text: "Dump tree"
            enabled: SocketConnector.connected

            onClicked: {
                treeModel.loadDump(SocketConnector.getDumpTree(filters))
                SocketConnector.getGrabWindow()
            }
        }

        Button {
            text: "Filters"
            enabled: SocketConnector.connected

            onClicked: {
                filtersPopup.show()
            }
        }

        Button {
            text: "Analyze"
            // enabled: SocketConnector.connected

            onClicked: {
                analyzeWindow.show()
            }
        }

        Button {
            Layout.rightMargin: 10
            text: "Expand tree"

            onClicked: {
                treeView.expandRecursively()
            }
        }
    }

    SplitView {
        id: splitView
        anchors.fill: parent
        anchors.topMargin: topLayout.height
        anchors.bottomMargin: bottomLayout.height

        handle: Item {
            id: handleDelegate
            width: 0
            height: 0

            Rectangle {
                implicitWidth: splitView.orientation === Qt.Horizontal ? 1 : splitView.width
                implicitHeight: splitView.orientation === Qt.Horizontal ? splitView.height : 1
                color: "black"
                opacity: 0.4
            }

            containmentMask: Item {
                x: (handleDelegate.width - width) / 2
                width: 8
                height: handleDelegate.height
            }
        }

        Item {
            id: leftLayout
            SplitView.minimumWidth: 300
            SplitView.preferredWidth: 600

            Image {
                id: screenshot
                anchors.fill: parent
                fillMode: Image.PreserveAspectFit
                property real scaleX: sourceSize.width / paintedWidth
                property real scaleY: sourceSize.height / paintedHeight

                function setFromB64(b64) {
                    source = "data:image/png;base64," + b64
                }

                MouseArea {
                    anchors.centerIn: parent
                    width: parent.paintedWidth
                    height: parent.paintedHeight
                    onClicked: {
                        const newIndex = treeModel.searchByCoordinates(mouseX * screenshot.scaleX, mouseY * screenshot.scaleY)
                        if (newIndex) {
                            treeView.selectByIndex(newIndex)
                        }
                    }

                    Rectangle {
                        id: selectionRect

                        color: "transparent"
                        border.width: 1
                        border.color: "#80ffde21"

                        function setRect(sRect) {
                            x = sRect.x / screenshot.scaleX
                            y = sRect.y / screenshot.scaleY
                            width = sRect.width / screenshot.scaleX
                            height = sRect.height / screenshot.scaleY
                        }
                    }
                }
            }
        }

        ColumnLayout {
            SplitView.minimumWidth: 300

            Row {
                id: headerRow
                x: -treeView.contentX
                height: 30
                Repeater {
                    model: treeModel.headers().length
                    Rectangle {
                        width: widths[index]
                        height: parent.height
                        color: "#eee"
                        border.color: "#aaa"
                        Text {
                            anchors.fill: parent
                            anchors.margins: 4
                            text: treeModel.headers()[index]
                            verticalAlignment: Text.AlignVCenter
                            font.bold: true
                            elide: Text.ElideRight
                        }
                        MouseArea {
                            anchors {
                                right: parent.right
                                top: parent.top
                                bottom: parent.bottom
                            }
                            width: 10
                            cursorShape: Qt.SplitHCursor
                            property real startX
                            onPressed: mouse => startX = mouse.x
                            onPositionChanged: mouse => {
                                var delta = mouse.x - startX
                                var newW = parent.width + delta
                                if (newW >= 30) {
                                    widths[index] = newW
                                    widthsChanged()
                                    treeView.forceLayout()
                                }
                            }
                            onDoubleClicked: {
                                const maxW = computeMaxWidth(index)
                                widths[index] = maxW
                                widthsChanged()
                                treeView.forceLayout()
                            }
                        }
                    }
                }
            }

            TreeView {
                id: treeView

                implicitWidth: parent.width
                Layout.topMargin: -5
                Layout.fillHeight: true
                clip: true

                property var selectedIndex: null
                property int searchIndex: 0

                onSelectedIndexChanged: {
                    const sRect = treeModel.getRect(selectedIndex)
                    selectionRect.setRect(sRect)
                }

                onWidthChanged: {
                    if (!loaded)
                        return
                    if (width > totalWidth) {
                        const nWidth = widths[0]
                        widths[0] = nWidth + (width - totalWidth)
                        widthsChanged()
                        treeView.forceLayout()
                    }
                }

                function selectByIndex(newIndex) {
                    treeView.expandToIndex(newIndex)
                    treeView.forceLayout()
                    treeView.positionViewAtRow(treeView.rowAtIndex(newIndex), Qt.AlignVCenter)
                    treeView.selectedIndex = newIndex
                }

                model: TreeModel {
                    id: treeModel
                }

                delegate: TreeViewDelegate {
                    id: delegate
                    clip: true

                    readonly property var modelIndex: treeView.modelIndex(Qt.point(column, row))

                    Rectangle {
                        anchors.fill: parent
                        color: treeView.selectedIndex
                               ? treeView.selectedIndex === delegate.modelIndex
                                 ? "#cce5ff"
                                 : treeView.selectedIndex.internalId == delegate.modelIndex.internalId
                                   ? "#e2eeff"
                                   : "transparent"
                               : "transparent"
                    }

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton | Qt.RightButton

                        onClicked: mouse => {
                            treeView.selectedIndex = delegate.modelIndex
                            if (mouse.button === Qt.RightButton) {
                                const props = treeModel.getDataVariant(delegate.modelIndex)
                                propsPopup.showData(props)
                            }
                        }

                        onDoubleClicked: {
                            clipboard.copyText(model.display)
                        }
                    }

                    MouseArea {
                        anchors {
                            right: parent.right
                            top: parent.top
                            bottom: parent.bottom
                        }
                        width: 10
                        cursorShape: Qt.SplitHCursor
                        property real startX
                        onPressed: mouse => {
                            treeView.interactive = false
                            startX = mouse.x
                        }
                        onPositionChanged: mouse => {
                            var delta = mouse.x - startX
                            var newW = parent.width + delta
                            if (newW >= 30) {
                                widths[column] = newW
                                widthsChanged()
                                treeView.forceLayout()
                            }
                        }
                        onDoubleClicked: {
                            const maxW = computeMaxWidth(index)
                            widths[column] = maxW
                            widthsChanged()
                            treeView.forceLayout()
                        }
                        onReleased: treeView.interactive = true
                    }
                }

                columnWidthProvider: column => widths[column]

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }
                ScrollBar.horizontal: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }
            }
        }
    }

    RowLayout {
        id: bottomLayout
        anchors.bottom: parent.bottom
        width: parent.width
        spacing: 10

        TextField {
            id: searchField

            Layout.leftMargin: 10
            Layout.fillWidth: true
            Layout.minimumWidth: 100
            leftPadding: 5
            rightPadding: 5

            placeholderText: "Enter search term"

            Keys.onReturnPressed: {
                if (text) {
                    searchButton.clicked(null)
                }
            }
        }

        CheckBox {
            id: partialCheckbox
            text: "Partial"
        }

        RadioButton {
            id: classNameRadio
            text: "ClassName"
            checked: true
            onClicked: treeView.searchIndex = 0
        }

        RadioButton {
            id: textRadio
            text: "Text"
            onClicked: treeView.searchIndex = 1
        }

        RadioButton {
            id: objectNameRadio
            text: "ObjectName"
            onClicked: treeView.searchIndex = 2
        }

        RadioButton {
            id: objectIdRadio
            text: "ObjectId"
            onClicked: treeView.searchIndex = 3
        }

        Button {
            id: searchButton
            Layout.rightMargin: 10
            text: "Search"
            enabled: SocketConnector.connected && searchField.text

            onClicked: {
                const nextIndex = treeModel.searchIndex(
                                    treeView.searchIndex,
                                    searchField.text,
                                    partialCheckbox.checked,
                                    treeView.selectedIndex ? treeView.selectedIndex : treeView.rootIndex)
                if (nextIndex) {
                    treeView.expandToIndex(nextIndex)
                    treeView.forceLayout()
                    treeView.positionViewAtRow(treeView.rowAtIndex(nextIndex), Qt.AlignVCenter)
                    treeView.selectedIndex = nextIndex
                }
            }
        }
    }

    Window {
        id: filtersPopup
        title: "Filters"

        width: 400
        height: minimumHeight

        minimumWidth: 300
        minimumHeight: filtersView.count * 30

        onVisibleChanged: {
            if (!visible)
                return

            filtersModel.printModelAsJson()
        }

        ListModel {
            id: filtersModel

            Component.onCompleted: {
                const jsonString = '[ \
                    { "key": "visible", "op": "eq", "value": "1" }, \
                    { "key": "enabled", "op": "eq", "value": "1" }, \
                    { "key": "opacity", "op": "gt", "value": "0" } \
                ]';

                const data = JSON.parse(filters ? filters : jsonString);

                for (let i = 0; i < data.length; i++) {
                    append(data[i]);
                }
            }

            function printModelAsJson() {
                let result = [];
                for (let i = 0; i < count; i++) {
                    result.push(get(i));
                }
                filters = JSON.stringify(result);
                if (SocketConnector.connected) {
                    treeModel.loadDump(SocketConnector.getDumpTree(filters))
                }
            }
        }

        ListView {
            id: filtersView
            anchors.fill: parent

            model: filtersModel

            header: Button {
                text: "Disable filters"
                padding: 8
                onClicked: {
                    filtersPopup.close()
                    filters = ""
                    treeModel.loadDump(SocketConnector.getDumpTree(filters))
                }
            }

            delegate: RowLayout {
                width: ListView.view.width
                implicitHeight: 30

                TextField {
                    id: filterKey
                    Layout.fillWidth: true
                    padding: 4
                    text: key
                    Keys.onReleased: {
                        filtersModel.setProperty(index, "key", text);
                        filtersModel.printModelAsJson();
                    }
                }

                ComboBox {
                    id: filterOperation
                    Layout.preferredWidth: 100
                    model: ListModel {
                        ListElement { text: "eq" }
                        ListElement { text: "ne" }
                        ListElement { text: "gt" }
                        ListElement { text: "lt" }
                    }
                    currentIndex: find(op)

                    Component.onCompleted: {
                        currentIndex = find(op)
                    }
                    onActivated: idx => {
                        filtersModel.setProperty(index, "op", currentText);
                        filtersModel.printModelAsJson();
                    }
                }

                TextField {
                    id: filterValue
                    Layout.fillWidth: true
                    padding: 4
                    text: value
                    Keys.onReleased: {
                        filtersModel.setProperty(index, "value", text);
                        filtersModel.printModelAsJson();
                    }
                }
            }
        }
    }

    Window {
        id: propsPopup
        title: "item properties"

        width: 350
        height: 600

        property var props: ({})
        property var keys: []

        function showData(d) {
            keys = []
            props = d
            keys = Object.keys(d)

            show()
        }

        Flickable {
            id: flick
            anchors.fill: parent
            clip: true
            flickableDirection: Flickable.VerticalFlick
            boundsBehavior: Flickable.StopAtBounds

            contentWidth: flick.width
            contentHeight: split.height

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            SplitView {
                id: split
                orientation: Qt.Horizontal
                width: flick.width

                implicitHeight: Math.max(...contentChildren.map(c => c.implicitHeight))

                handle: Item {
                    id: handlePropsDelegate
                    width: 0
                    height: 0

                    Rectangle {
                        implicitWidth: split.orientation === Qt.Horizontal ? 1 : split.width
                        implicitHeight: split.orientation === Qt.Horizontal ? split.height : 1
                        color: "black"
                        opacity: 0.4
                    }

                    containmentMask: Item {
                        x: (handlePropsDelegate.width - width) / 2
                        width: 8
                        height: handlePropsDelegate.height
                    }
                }

                Item {
                    implicitHeight: keysColumn.height
                    SplitView.minimumWidth: 150
                    SplitView.preferredWidth: 150

                    Column {
                        id: keysColumn
                        width: parent.width
                        Repeater {
                            model: propsPopup.keys
                            TextField {
                                id: propsKey
                                width: parent.width
                                readOnly: false
                                text: modelData
                                leftPadding: 4
                                rightPadding: 4

                                onTextChanged: cursorPosition = 0

                                onFocusChanged: {
                                    if (focus) {
                                        selectAll()
                                    }
                                }
                            }
                        }
                    }
                }

                Item {
                    implicitHeight: valuesColumn.height
                    SplitView.minimumWidth: 200

                    Column {
                        id: valuesColumn
                        width: parent.width
                        Repeater {
                            model: propsPopup.keys
                            TextField {
                                id: propsValue
                                width: parent.width
                                       - (flick.ScrollBar.vertical.visible ? flick.ScrollBar.vertical.width : 0)
                                readOnly: false
                                text: propsPopup.props[modelData]
                                leftPadding: 4
                                rightPadding: 4

                                onTextChanged: cursorPosition = 0

                                onFocusChanged: {
                                    if (focus) {
                                        selectAll()
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Window {
        id: analyzeWindow

        width: minimumWidth
        minimumWidth: 400
        height: 800

        title: "Analyze tool"

        transientParent: null

        onVisibleChanged: {
            if (!visible)
                return

            if (SocketConnector.connected)
                SocketConnector.startAnalyze()

            analyzeModel.clear()
            SocketConnector.manager.load()
        }

        onClosing: {
            if (SocketConnector.connected)
                SocketConnector.stopAnalyze()
        }

        Connections {
            target: SocketConnector.manager

            function onDataAdded(p, loc) {
                analyzeModel.append({ pos: p, location: loc })
            }
        }

        ListView {
            anchors.fill: parent

            model: analyzeModel
            spacing: 8

            delegate: MouseArea {
                width: ListView.view.width
                height: 80

                acceptedButtons: Qt.LeftButton | Qt.RightButton

                onClicked: mouse => {
                    if (mouse.button === Qt.RightButton) {
                        menu.popup()
                        return
                    }
                    screenshot.source = "file:///" + model.location + "/screenshot.png"
                    treeModel.loadFile(model.location + "/dump.json")
                    const idx = treeModel.searchByCoordinates(model.pos.x, model.pos.y)
                    if (idx) {
                        treeView.selectByIndex(idx)
                        treeView.positionViewAtRow(treeView.rowAtIndex(idx), Qt.AlignVCenter)
                    }
                }

                Menu {
                    id: menu

                    MenuItem {
                        text: "Refine"
                    }
                    MenuItem {
                        text: "Delete"
                        onClicked: {
                            SocketConnector.manager.remove(model.location)
                            analyzeModel.remove(index)
                        }
                    }
                }

                RowLayout {
                    anchors.fill: parent

                    Text {
                        Layout.fillWidth: true
                        text: Qt.formatDateTime(new Date(parseInt(model.location.split('/').pop())), "yyyy-MM-dd hh:mm:ss")
                        padding: 4
                    }

                    Image {
                        height: parent.height
                        sourceSize.height: height
                        source: "file:///" + model.location + "/screenshot.png"
                        cache: true
                        fillMode: Image.PreserveAspectFit
                    }
                }
            }
        }

        ListModel {
            id: analyzeModel
        }
    }
}
