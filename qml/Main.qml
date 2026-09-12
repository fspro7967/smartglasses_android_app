import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs as QDialogs

ApplicationWindow {
    id: root
    visible: true
    width: 420
    height: 820
    minimumWidth: 360
    minimumHeight: 600
    title: qsTr("智能眼镜助手")
    color: cBg

    // ==================== 主题色 ====================
    readonly property color cAccent: "#4F8CFF"
    readonly property color cBg: "#0B0E14"
    readonly property color cCard: "#151A24"
    readonly property color cBorder: "#232B3A"
    readonly property color cText: "#EDF0F5"
    readonly property color cSubText: "#8B93A5"
    readonly property color cGreen: "#34C77B"
    readonly property color cYellow: "#E5B93C"
    readonly property color cRed: "#E5484D"

    // ==================== 数据模型 ====================
    ListModel { id: deviceModel }
    ListModel { id: logModel }

    property string transcript: ""
    property string aiReply: ""
    // 主内容区标签页索引（0 = 语音识别，1 = AI 回复）
    property int mainTabIndex: 0
    // 收到新 AI 回复但用户仍停留在其他标签页时的未读标记
    property bool aiUnread: false
    // 当前选中的设备索引（ListView.currentIndex 点击不会自动更新，需显式维护）
    property int selectedDeviceIndex: -1
    // 指定设备是否处于已连接状态
    function isDeviceConnected(index) {
        return backend.connected && index === root.selectedDeviceIndex
    }

    // 当前 AI 接入方式显示文本（API / Ollama）
    property string aiModeText: "API · " + backend.apiModelName()
    function updateAiModeText() {
        root.aiModeText = backend.aiProvider() === 1
            ? "Ollama · " + backend.ollamaModelName()
            : "API · " + backend.apiModelName()
    }

    // 选择音频文件并送入 Whisper 识别
    QDialogs.FileDialog {
        id: audioDialog
        title: qsTr("选择音频文件")
        nameFilters: [qsTr("音频文件 (*.wav *.pcm)"), qsTr("所有文件 (*)")]
        onAccepted: backend.processAudioFile(selectedFile.toString().replace(/^file:\/\//, ""))
    }

    // ==================== 后端信号 -> QML ====================
    Connections {
        target: backend

        function onDeviceDiscovered(name) {
            deviceModel.append({ name: name })
        }

        function onStatusMessage(msg) {
            logModel.append({ text: msg, level: msg.includes("错误") || msg.includes("失败") ? "error" : "info" })
            trimLog()
        }

        function onTranscriptionReady(text) {
            root.transcript = text
            logModel.append({ text: "[识别] " + text, level: "transcript" })
            trimLog()
        }

        function onAiResponseReady(text) {
            root.aiReply = text
            if (root.mainTabIndex !== 1)
                root.aiUnread = true
            logModel.append({ text: "[AI] " + text, level: "ai" })
            trimLog()
        }

        function onAiConfigChanged() {
            root.updateAiModeText()
        }
    }

    function trimLog() {
        if (logModel.count > 300)
            logModel.remove(0, logModel.count - 300)
        logView.positionViewAtEnd()
    }

    // ==================== 顶部工具栏 ====================
    header: ToolBar {
        height: 56
        background: Rectangle { color: cCard; border.color: cBorder; border.width: 1 }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 8
            spacing: 8

            ColumnLayout {
                spacing: 1
                Layout.fillWidth: true
                Text {
                    text: qsTr("智能眼镜助手")
                    color: cText
                    font.pixelSize: 17
                    font.bold: true
                }
                Text {
                    text: root.connectionStatusText
                    color: root.connectionStatusColor
                    font.pixelSize: 12
                }
            }

            Button {
                text: backend.scanning ? qsTr("扫描中...") : qsTr("扫描设备")
                enabled: !backend.scanning
                onClicked: {
                    deviceModel.clear()
                    root.selectedDeviceIndex = -1
                    backend.startScan()
                }
                contentItem: Text {
                    text: parent.text
                    color: parent.enabled ? "#FFFFFF" : cSubText
                    font.pixelSize: 13
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 8
                    color: parent.enabled ? cAccent : "#33405A"
                }
                implicitHeight: 36
                implicitWidth: 88
            }

            Button {
                text: qsTr("选音频")
                onClicked: audioDialog.open()
                contentItem: Text {
                    text: parent.text
                    color: cText
                    font.pixelSize: 13
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 8
                    color: cBorder
                }
                implicitHeight: 36
                implicitWidth: 76
            }

            // ---- 右上角设置菜单（AI 大模型接入方式） ----
            Button {
                id: moreBtn
                text: "⋮"
                onClicked: settingsMenu.popup()
                contentItem: Text {
                    text: parent.text
                    color: cText
                    font.pixelSize: 20
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 8
                    color: moreBtn.down ? "#2A3342" : cBorder
                }
                implicitHeight: 36
                implicitWidth: 40

                Menu {
                    id: settingsMenu
                    x: parent.width - width
                    y: parent.height + 4
                    transformOrigin: Menu.TopRight
                    width: 240

                    // 深色主题
                    palette {
                        window: cCard
                        windowText: cText
                        base: cCard
                        alternateBase: cCard
                        text: cText
                        button: cCard
                        buttonText: cText
                        highlight: cAccent
                        highlightedText: "#FFFFFF"
                        toolTipBase: cCard
                        toolTipText: cText
                    }
                    background: Rectangle {
                        color: cCard
                        border.color: cBorder
                        border.width: 1
                        radius: 10
                    }

                    MenuItem {
                        enabled: false
                        height: 30
                        contentItem: Text {
                            text: qsTr("AI 大模型接入方式")
                            color: cSubText
                            font.pixelSize: 12
                            font.bold: true
                            leftPadding: 12
                        }
                    }
                    MenuSeparator {
                        contentItem: Rectangle {
                            implicitWidth: parent.width
                            implicitHeight: 1
                            color: cBorder
                        }
                    }

                    MenuItem {
                        text: qsTr("OpenAI 兼容 API 配置")
                        highlighted: backend.aiProvider() === 0
                        onClicked: apiConfigDialog.open()
                        contentItem: Text {
                            text: parent.text
                            color: parent.highlighted ? "#FFFFFF" : cText
                            font.pixelSize: 13
                            leftPadding: 12
                        }
                        background: Rectangle {
                            color: parent.highlighted ? cAccent
                                 : (parent.hovered ? "#1E2532" : "transparent")
                        }
                    }
                    MenuItem {
                        text: qsTr("本地 Ollama 部署")
                        highlighted: backend.aiProvider() === 1
                        onClicked: ollamaConfigDialog.open()
                        contentItem: Text {
                            text: parent.text
                            color: parent.highlighted ? "#FFFFFF" : cText
                            font.pixelSize: 13
                            leftPadding: 12
                        }
                        background: Rectangle {
                            color: parent.highlighted ? cAccent
                                 : (parent.hovered ? "#1E2532" : "transparent")
                        }
                    }

                    MenuSeparator {
                        contentItem: Rectangle {
                            implicitWidth: parent.width
                            implicitHeight: 1
                            color: cBorder
                        }
                    }
                    MenuItem {
                        enabled: false
                        height: 30
                        contentItem: Text {
                            text: qsTr("当前接入: ") + root.aiModeText
                            color: cSubText
                            font.pixelSize: 12
                            leftPadding: 12
                        }
                    }
                }
            }
        }
    }

    // ==================== 主内容 ====================
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        // ---- 设备列表 ----
        Rectangle {
            visible: deviceModel.count > 0
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(150, 44 + deviceModel.count * 44)
            radius: 12
            color: cCard
            border.color: cBorder

            ColumnLayout {
                anchors.fill: parent
                spacing: 0
                Text {
                    text: qsTr("附近设备 (%1)").arg(deviceModel.count)
                    color: cSubText
                    font.pixelSize: 12
                    leftPadding: 12
                    topPadding: 8
                    bottomPadding: 4
                }
                ListView {
                    id: deviceView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: deviceModel
                    delegate: ItemDelegate {
                        width: ListView.view.width
                        height: 40
                        onClicked: {
                            if (backend.connected && index === root.selectedDeviceIndex) {
                                backend.disconnectDevice()
                            } else {
                                root.selectedDeviceIndex = index
                                backend.connectToDevice(index)
                            }
                        }
                        contentItem: RowLayout {
                            spacing: 8
                            Rectangle {
                                implicitWidth: 8; implicitHeight: 8; radius: 4
                                color: root.isDeviceConnected(index) ? cGreen : cSubText
                            }
                            Text {
                                text: model.name
                                color: cText
                                font.pixelSize: 14
                                elide: Text.ElideMiddle
                                Layout.fillWidth: true
                            }
                            Text {
                                text: root.isDeviceConnected(index) ? qsTr("断开") : qsTr("连接")
                                color: root.isDeviceConnected(index) ? cGreen : cAccent
                                font.pixelSize: 12
                            }
                        }
                    }
                }
            }
        }

        // ---- 服务/特征列表（数据由 C++ 端 backend.services 提供） ----
        Rectangle {
            visible: backend.services.length > 0
            Layout.fillWidth: true
            Layout.preferredHeight: servicesExpanded
                ? serviceHeaderH + Math.min(serviceContent.contentHeight, 200)
                : serviceHeaderH
            radius: 12
            color: cCard
            border.color: cBorder

            readonly property int serviceHeaderH: 36

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // 标题行（点击展开/折叠）
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: parent.serviceHeaderH
                    color: "transparent"
                    MouseArea {
                        anchors.fill: parent
                        onClicked: servicesExpanded = !servicesExpanded
                    }
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 8
                        Text {
                            text: qsTr("蓝牙服务 (%1)").arg(backend.services.length)
                            color: cSubText
                            font.pixelSize: 12
                            Layout.fillWidth: true
                        }
                        Text {
                            text: servicesExpanded ? "▾" : "▸"
                            color: cSubText
                            font.pixelSize: 12
                        }
                    }
                }

                ListView {
                    id: serviceContent
                    visible: servicesExpanded
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.min(contentHeight, 200)
                    clip: true
                    model: backend.services
                    delegate: Column {
                        width: ListView.view.width
                        spacing: 2

                        // 服务行
                        RowLayout {
                            width: parent.width
                            height: 28
                            spacing: 6
                            Rectangle {
                                implicitWidth: 8; implicitHeight: 8; radius: 4
                                color: cAccent
                            }
                            Text {
                                text: qsTr("服务")
                                color: cAccent
                                font.pixelSize: 11
                            }
                            Text {
                                text: modelData.label
                                color: cText
                                font.pixelSize: 11
                                elide: Text.ElideMiddle
                                Layout.fillWidth: true
                            }
                        }

                        // 特征行
                        Repeater {
                            model: modelData.chars
                            delegate: Rectangle {
                                width: parent.width
                                height: 44
                                color: mouse.containsMouse ? "#1E2532" : "transparent"
                                radius: 8
                                MouseArea {
                                    id: mouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onClicked: {
                                        if (modelData.notifiable)
                                            backend.enableNotification(modelData.serviceUuid, modelData.charUuid)
                                    }
                                }
                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 24
                                    anchors.rightMargin: 8
                                    spacing: 8
                                    ColumnLayout {
                                        spacing: 2
                                        Layout.fillWidth: true
                                        Text {
                                            text: modelData.charName.length > 0 ? modelData.charName : modelData.charUuid
                                            color: cText
                                            font.pixelSize: 13
                                            elide: Text.ElideMiddle
                                            Layout.fillWidth: true
                                        }
                                        Text {
                                            text: modelData.charUuid + "  " + modelData.props
                                            color: cSubText
                                            font.pixelSize: 10
                                            elide: Text.ElideMiddle
                                            Layout.fillWidth: true
                                        }
                                    }
                                    Text {
                                        visible: modelData.notifiable
                                        text: qsTr("启用通知")
                                        color: cAccent
                                        font.pixelSize: 11
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // ---- 识别与 AI 回复（主显示区，标签页切换） ----
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 12
            color: cCard
            border.color: cBorder

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 10

                // 标签页切换
                Row {
                    spacing: 0

                    // 语音识别标签
                    ItemDelegate {
                        width: tabText0.implicitWidth + 16
                        height: 30
                        padding: 0
                        background: Rectangle { color: "transparent" }
                        onClicked: root.mainTabIndex = 0
                        contentItem: Column {
                            spacing: 0
                            Text {
                                id: tabText0
                                text: qsTr("语音识别")
                                color: root.mainTabIndex === 0 ? cText : cSubText
                                font.pixelSize: 13
                                font.bold: root.mainTabIndex === 0
                            }
                            Rectangle {
                                width: parent.width
                                height: 2
                                radius: 1
                                color: root.mainTabIndex === 0 ? cAccent : "transparent"
                            }
                        }
                    }

                    // AI 回复标签（新回复到达时显示红点）
                    ItemDelegate {
                        width: tabText1.implicitWidth + 16 + (root.aiUnread ? 16 : 0)
                        height: 30
                        padding: 0
                        background: Rectangle { color: "transparent" }
                        onClicked: {
                            root.mainTabIndex = 1
                            root.aiUnread = false
                        }
                        contentItem: Column {
                            spacing: 0
                            Row {
                                spacing: 4
                                Text {
                                    id: tabText1
                                    text: qsTr("AI 回复")
                                    color: root.mainTabIndex === 1 ? cText : cSubText
                                    font.pixelSize: 13
                                    font.bold: root.mainTabIndex === 1
                                }
                                Rectangle {
                                    visible: root.aiUnread
                                    width: 8
                                    height: 8
                                    radius: 4
                                    color: cRed
                                }
                            }
                            Rectangle {
                                width: parent.width
                                height: 2
                                radius: 1
                                color: root.mainTabIndex === 1 ? cAccent : "transparent"
                            }
                        }
                    }
                }

                // 内容区：两个页面共用同一位置
                StackLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: root.mainTabIndex

                    // 页 0：语音识别字幕
                    ScrollView {
                        ScrollBar.vertical.policy: ScrollBar.AsNeeded
                        Text {
                            width: parent.width
                            text: transcript.length > 0 ? transcript : qsTr("等待接收语音...")
                            color: transcript.length > 0 ? cText : cSubText
                            font.pixelSize: 22
                            font.bold: transcript.length > 0
                            wrapMode: Text.Wrap
                            textFormat: Text.PlainText
                            verticalAlignment: Text.AlignTop
                        }
                    }

                    // 页 1：AI 回复
                    ColumnLayout {
                        spacing: 8

                        ScrollView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            ScrollBar.vertical.policy: ScrollBar.AsNeeded
                            Text {
                                width: parent.width
                                text: aiReply.length > 0 ? aiReply : qsTr("等待 AI 回复...")
                                color: aiReply.length > 0 ? cText : cSubText
                                font.pixelSize: 16
                                wrapMode: Text.Wrap
                                textFormat: Text.PlainText
                                verticalAlignment: Text.AlignTop
                            }
                        }

                        // 朗读控制：回复到达后自动朗读，这里只提供「停止」与「重播」。
                        // 停止是立即生效的（后端会丢弃已缓冲音频）。
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            visible: aiReply.length > 0

                            Button {
                                text: qsTr("停止")
                                Layout.fillWidth: true
                                implicitHeight: 36
                                enabled: backend.speaking
                                onClicked: backend.stopSpeaking()
                                contentItem: Text {
                                    text: parent.text
                                    color: backend.speaking ? cText : cSubText
                                    font.pixelSize: 14
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                background: Rectangle { radius: 8; color: cBorder }
                            }

                            Button {
                                text: qsTr("重播")
                                Layout.fillWidth: true
                                implicitHeight: 36
                                enabled: backend.ttsReady
                                onClicked: backend.replayLastReply()
                                contentItem: Text {
                                    text: parent.text
                                    color: backend.ttsReady ? "#FFFFFF" : cSubText
                                    font.pixelSize: 14
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                background: Rectangle {
                                    radius: 8
                                    color: backend.ttsReady ? cAccent : cBorder
                                }
                            }
                        }
                    }
                }
            }
        }

        // ---- 状态日志 ----
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 140
            radius: 12
            color: cCard
            border.color: cBorder

            ColumnLayout {
                anchors.fill: parent
                spacing: 0
                Text {
                    text: qsTr("运行日志")
                    color: cSubText
                    font.pixelSize: 12
                    leftPadding: 12
                    topPadding: 8
                    bottomPadding: 4
                }
                ListView {
                    id: logView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 2
                    model: logModel
                    delegate: Text {
                        width: ListView.view.width
                        leftPadding: 12
                        rightPadding: 12
                        text: model.text
                        color: model.level === "error" ? cRed
                             : model.level === "ai" ? cAccent
                             : model.level === "transcript" ? cText
                             : cSubText
                        font.pixelSize: 11
                        wrapMode: Text.Wrap
                    }
                }
            }
        }
    }

    // 连接状态文字与颜色
    readonly property string connectionStatusText: {
        if (backend.connected) return qsTr("已连接: ") + backend.deviceName
        if (backend.scanning) return qsTr("正在扫描设备...")
        return qsTr("未连接")
    }
    readonly property color connectionStatusColor: {
        if (backend.connected) return cGreen
        if (backend.scanning) return cYellow
        return cSubText
    }

    property bool servicesExpanded: true

    // ==================== OpenAI 兼容 API 配置对话框 ====================
    Dialog {
        id: apiConfigDialog
        modal: true
        anchors.centerIn: parent
        width: Math.min(parent.width - 40, 380)
        padding: 0
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        standardButtons: Dialog.NoButton
        background: Rectangle {
            color: cCard
            border.color: cBorder
            border.width: 1
            radius: 12
        }

        contentItem: ColumnLayout {
            spacing: 0

            // 标题
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                color: "transparent"
                Text {
                    anchors.fill: parent
                    anchors.leftMargin: 20
                    verticalAlignment: Text.AlignVCenter
                    text: qsTr("OpenAI 兼容 API 配置")
                    color: cText
                    font.pixelSize: 16
                    font.bold: true
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 20
                Layout.rightMargin: 20
                Layout.bottomMargin: 16
                spacing: 8

                Text { text: qsTr("API_BASE_URL"); color: cSubText; font.pixelSize: 12 }
                TextField {
                    id: apiBaseUrlField
                    Layout.fillWidth: true
                    text: backend.apiBaseUrl()
                    placeholderText: ""
                    placeholderTextColor: cSubText
                    selectByMouse: true
                    leftPadding: 12
                    rightPadding: 12
                    font.pixelSize: 14
                    color: cText
                    background: Rectangle {
                        color: "#1E2532"
                        border.color: apiBaseUrlField.activeFocus ? cAccent : cBorder
                        border.width: 1
                        radius: 8
                        implicitHeight: 44
                    }
                }

                Text { text: qsTr("API_KEY"); color: cSubText; font.pixelSize: 12 }
                TextField {
                    id: apiKeyField
                    Layout.fillWidth: true
                    text: backend.apiKey()
                    placeholderText: ""
                    placeholderTextColor: cSubText
                    echoMode: TextInput.Password
                    selectByMouse: true
                    leftPadding: 12
                    rightPadding: 12
                    font.pixelSize: 14
                    color: cText
                    background: Rectangle {
                        color: "#1E2532"
                        border.color: apiKeyField.activeFocus ? cAccent : cBorder
                        border.width: 1
                        radius: 8
                        implicitHeight: 44
                    }
                }

                Text { text: qsTr("MODEL_NAME"); color: cSubText; font.pixelSize: 12 }
                TextField {
                    id: apiModelField
                    Layout.fillWidth: true
                    text: backend.apiModelName()
                    placeholderText: ""
                    placeholderTextColor: cSubText
                    selectByMouse: true
                    leftPadding: 12
                    rightPadding: 12
                    font.pixelSize: 14
                    color: cText
                    background: Rectangle {
                        color: "#1E2532"
                        border.color: apiModelField.activeFocus ? cAccent : cBorder
                        border.width: 1
                        radius: 8
                        implicitHeight: 44
                    }
                }

                Text {
                    id: apiConfigError
                    visible: false
                    color: cRed
                    font.pixelSize: 12
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 4
                    spacing: 10

                    Button {
                        text: qsTr("取消")
                        Layout.fillWidth: true
                        implicitHeight: 40
                        onClicked: apiConfigDialog.close()
                        contentItem: Text {
                            text: parent.text
                            color: cText
                            font.pixelSize: 14
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle { radius: 8; color: cBorder }
                    }
                    Button {
                        text: qsTr("保存并启用")
                        Layout.fillWidth: true
                        implicitHeight: 40
                        onClicked: {
                            var baseUrl = apiBaseUrlField.text.trim()
                            var apiKey  = apiKeyField.text.trim()
                            var model   = apiModelField.text.trim()
                            if (baseUrl === "" || apiKey === "" || model === "") {
                                apiConfigError.text = qsTr("请完整填写 API_BASE_URL、API_KEY、MODEL_NAME 三个配置项")
                                apiConfigError.visible = true
                                return
                            }
                            backend.setApiConfig(baseUrl, apiKey, model)
                            backend.setAiProvider(0)
                            apiConfigError.visible = false
                            apiConfigDialog.close()
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "#FFFFFF"
                            font.pixelSize: 14
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle { radius: 8; color: cAccent }
                    }
                }
            }
        }
    }

    // ==================== 本地 Ollama 配置对话框 ====================
    Dialog {
        id: ollamaConfigDialog
        modal: true
        anchors.centerIn: parent
        width: Math.min(parent.width - 40, 380)
        padding: 0
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        standardButtons: Dialog.NoButton
        background: Rectangle {
            color: cCard
            border.color: cBorder
            border.width: 1
            radius: 12
        }

        contentItem: ColumnLayout {
            spacing: 0

            // 标题
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                color: "transparent"
                Text {
                    anchors.fill: parent
                    anchors.leftMargin: 20
                    verticalAlignment: Text.AlignVCenter
                    text: qsTr("本地 Ollama 配置")
                    color: cText
                    font.pixelSize: 16
                    font.bold: true
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 20
                Layout.rightMargin: 20
                Layout.bottomMargin: 16
                spacing: 8

                Text {
                    text: qsTr("请在电脑上安装并运行 Ollama（终端执行 ollama serve），")
                    color: cSubText
                    font.pixelSize: 11
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
                Text {
                    text: qsTr("手机与电脑需处于同一局域网，地址请填写电脑的局域网 IP。")
                    color: cSubText
                    font.pixelSize: 11
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }

                Text { text: qsTr("服务器地址"); color: cSubText; font.pixelSize: 12 }
                TextField {
                    id: ollamaUrlField
                    Layout.fillWidth: true
                    text: backend.ollamaUrl()
                    placeholderText: ""
                    placeholderTextColor: cSubText
                    selectByMouse: true
                    leftPadding: 12
                    rightPadding: 12
                    font.pixelSize: 14
                    color: cText
                    background: Rectangle {
                        color: "#1E2532"
                        border.color: ollamaUrlField.activeFocus ? cAccent : cBorder
                        border.width: 1
                        radius: 8
                        implicitHeight: 44
                    }
                }

                Text { text: qsTr("模型名称"); color: cSubText; font.pixelSize: 12 }
                TextField {
                    id: ollamaModelField
                    Layout.fillWidth: true
                    text: backend.ollamaModelName()
                    placeholderText: ""
                    placeholderTextColor: cSubText
                    selectByMouse: true
                    leftPadding: 12
                    rightPadding: 12
                    font.pixelSize: 14
                    color: cText
                    background: Rectangle {
                        color: "#1E2532"
                        border.color: ollamaModelField.activeFocus ? cAccent : cBorder
                        border.width: 1
                        radius: 8
                        implicitHeight: 44
                    }
                }

                Text {
                    id: ollamaConfigError
                    visible: false
                    color: cRed
                    font.pixelSize: 12
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 4
                    spacing: 10

                    Button {
                        text: qsTr("取消")
                        Layout.fillWidth: true
                        implicitHeight: 40
                        onClicked: ollamaConfigDialog.close()
                        contentItem: Text {
                            text: parent.text
                            color: cText
                            font.pixelSize: 14
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle { radius: 8; color: cBorder }
                    }
                    Button {
                        text: qsTr("保存并启用")
                        Layout.fillWidth: true
                        implicitHeight: 40
                        onClicked: {
                            var serverUrl = ollamaUrlField.text.trim()
                            var model     = ollamaModelField.text.trim()
                            if (serverUrl === "" || model === "") {
                                ollamaConfigError.text = qsTr("请完整填写服务器地址与模型名称")
                                ollamaConfigError.visible = true
                                return
                            }
                            backend.setOllamaConfig(serverUrl, model)
                            backend.setAiProvider(1)
                            ollamaConfigError.visible = false
                            ollamaConfigDialog.close()
                        }
                        contentItem: Text {
                            text: parent.text
                            color: "#FFFFFF"
                            font.pixelSize: 14
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle { radius: 8; color: cAccent }
                    }
                }
            }
        }
    }
}
