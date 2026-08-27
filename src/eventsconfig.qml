import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kcmutils as KCMUtils
import org.kde.plasma.private.locallunarcalendar 1.0

KCMUtils.SimpleKCM {
    id: root

    property string editingId: ""
    property string formName: ""
    property string formDescription: ""
    property string formDate: ""
    property string formColor: "#8e24aa"
    property string formRepeatType: "none"
    property int formRepeatInterval: 1
    property string formRepeatUnit: "day"
    property bool editingForm: false
    property var repeatTypeValues: ["none", "daily", "weekly", "monthly", "yearly", "custom"]
    property var repeatTypeLabels: ["不重复", "每天", "每周", "每月", "每年", "自定义"]
    property var repeatUnitValues: ["day", "week", "month", "year"]
    property var repeatUnitLabels: ["天", "周", "月", "年"]
    property string pendingDeleteId: ""
    property string pendingDeleteName: ""

    function isoDate(date) {
        return Qt.formatDate(date, "yyyy-MM-dd")
    }

    function openAdd() {
        editingId = ""
        formName = ""
        formDescription = ""
        formDate = isoDate(new Date())
        formColor = "#8e24aa"
        formRepeatType = "none"
        formRepeatInterval = 1
        formRepeatUnit = "day"
        eventsModel.clearError()
        editingForm = true
    }

    function openEdit(eventId, eventName, eventDescription, eventDate, eventColor, repeatType, repeatInterval, repeatUnit) {
        editingId = eventId
        formName = eventName
        formDescription = eventDescription
        formDate = eventDate
        formColor = eventColor
        formRepeatType = repeatType
        formRepeatInterval = repeatInterval
        formRepeatUnit = repeatUnit
        eventsModel.clearError()
        editingForm = true
    }

    function repeatTypeIndex(value) {
        var index = repeatTypeValues.indexOf(value)
        return index >= 0 ? index : 0
    }

    function repeatUnitIndex(value) {
        var index = repeatUnitValues.indexOf(value)
        return index >= 0 ? index : 0
    }

    function repeatDescription(repeatType, repeatInterval, repeatUnit) {
        var typeIndex = repeatTypeValues.indexOf(repeatType)
        if (repeatType === "custom") {
            var unitIndex = repeatUnitValues.indexOf(repeatUnit)
            var unitLabel = unitIndex >= 0 ? repeatUnitLabels[unitIndex] : "天"
            return "每 " + repeatInterval + " " + unitLabel
        }
        return typeIndex >= 0 ? repeatTypeLabels[typeIndex] : "不重复"
    }

    function tomorrow() {
        var date = new Date()
        date.setDate(date.getDate() + 1)
        return isoDate(date)
    }

    function closeEditor() {
        editingForm = false
        eventsModel.clearError()
    }

    function submitForm() {
        eventsModel.clearError()
        var saved
        if (editingId.length === 0) {
            saved = eventsModel.addEvent(formName,
                                         formDescription,
                                         formDate,
                                         formColor,
                                         formRepeatType,
                                         formRepeatInterval,
                                         formRepeatUnit)
        } else {
            saved = eventsModel.updateEvent(editingId,
                                            formName,
                                            formDescription,
                                            formDate,
                                            formColor,
                                            formRepeatType,
                                            formRepeatInterval,
                                            formRepeatUnit)
        }
        if (saved) {
            closeEditor()
        }
    }

    function requestDelete(eventId, eventName) {
        pendingDeleteId = eventId
        pendingDeleteName = eventName
        eventsModel.clearError()
        deleteDialog.open()
    }

    CustomEventsModel {
        id: eventsModel
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: Kirigami.Units.largeSpacing

        Controls.Label {
            text: root.editingForm
                  ? (root.editingId.length === 0 ? "添加事件" : "编辑事件")
                  : "自定义事件"
            font.bold: true
            Layout.fillWidth: true
        }

        Controls.Label {
            visible: !root.editingForm
            text: eventsModel.usingSystemDefaults
                  ? "当前使用系统预置事件；第一次编辑时会复制到用户文件。"
                  : "事件保存在用户文件中。"
            color: Kirigami.Theme.disabledTextColor
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Controls.CheckBox {
            visible: !root.editingForm
            text: "在悬浮提示中显示英文月份和星期"
            checked: eventsModel.showEnglishDate
            onToggled: eventsModel.showEnglishDate = checked
            Layout.fillWidth: true
        }

        StackLayout {
            id: contentStack

            currentIndex: root.editingForm ? 1 : 0
            Layout.fillWidth: true

            ColumnLayout {
                spacing: Kirigami.Units.smallSpacing

                ListView {
                    id: eventList

                    Layout.fillWidth: true
                    // 页面整体可滚动（SimpleKCM），列表随内容增长、超过 18 个网格单位后由页面滚动。
                    implicitHeight: Math.max(Kirigami.Units.gridUnit * 6, Math.min(contentHeight, Kirigami.Units.gridUnit * 18))
                    clip: true
                    spacing: 1
                    model: eventsModel

                    delegate: Controls.ItemDelegate {
                        width: eventList.width
                        height: Math.max(Kirigami.Units.gridUnit * 3, implicitHeight)

                        contentItem: RowLayout {
                            spacing: Kirigami.Units.smallSpacing

                            Rectangle {
                                Layout.alignment: Qt.AlignVCenter
                                width: Kirigami.Units.gridUnit
                                height: width
                                radius: width / 2
                                color: model.color
                                border.width: 1
                                border.color: Kirigami.Theme.disabledTextColor
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 0

                                Controls.Label {
                                    text: model.name
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                Controls.Label {
                                    text: model.date + " · "
                                          + root.repeatDescription(model.repeatType,
                                                                   model.repeatInterval,
                                                                   model.repeatUnit)
                                    color: Kirigami.Theme.disabledTextColor
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }

                            Controls.ToolButton {
                                icon.name: "document-edit"
                                display: Controls.AbstractButton.IconOnly
                                onClicked: root.openEdit(model.id,
                                                         model.name,
                                                         model.description,
                                                         model.date,
                                                         model.color,
                                                         model.repeatType,
                                                         model.repeatInterval,
                                                         model.repeatUnit)
                                Controls.ToolTip.visible: hovered
                                Controls.ToolTip.text: "编辑"
                            }

                            Controls.ToolButton {
                                icon.name: "edit-delete"
                                display: Controls.AbstractButton.IconOnly
                                onClicked: root.requestDelete(model.id, model.name)
                                Controls.ToolTip.visible: hovered
                                Controls.ToolTip.text: "删除"
                            }
                        }

                        onClicked: root.openEdit(model.id,
                                                 model.name,
                                                 model.description,
                                                 model.date,
                                                 model.color,
                                                 model.repeatType,
                                                 model.repeatInterval,
                                                 model.repeatUnit)
                    }
                }

                Controls.Label {
                    visible: eventList.count === 0
                    text: "暂无自定义事件"
                    color: Kirigami.Theme.disabledTextColor
                    horizontalAlignment: Text.AlignHCenter
                    Layout.fillWidth: true
                }

                Controls.Label {
                    visible: eventsModel.errorString.length > 0
                    text: eventsModel.errorString
                    color: Kirigami.Theme.negativeTextColor
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    Controls.Button {
                        icon.name: "list-add"
                        text: "添加事件"
                        onClicked: root.openAdd()
                    }

                    Controls.Button {
                        visible: eventsModel.hasUserFile
                        icon.name: "edit-undo"
                        text: "恢复系统预置"
                        onClicked: resetDialog.open()
                    }

                    Item {
                        Layout.fillWidth: true
                    }
                }
            }

            ColumnLayout {
                id: editorView

                spacing: Kirigami.Units.smallSpacing

                Item {
                    id: formArea

                    Layout.fillWidth: true
                    implicitHeight: editorForm.implicitHeight

                    Kirigami.FormLayout {
                        id: editorForm

                        anchors.top: parent.top
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: Math.min(parent.width, Kirigami.Units.gridUnit * 32)
                        height: implicitHeight

                        Controls.TextField {
                            id: nameField

                            Kirigami.FormData.label: "名称"
                            text: root.formName
                            placeholderText: "例如：爸爸生日"
                            onTextEdited: root.formName = text
                            Layout.fillWidth: true
                        }

                        Controls.ScrollView {
                            id: descriptionView

                            Kirigami.FormData.label: "详情"
                            Layout.fillWidth: true
                            implicitHeight: Kirigami.Units.gridUnit * 5
                            Layout.preferredHeight: Kirigami.Units.gridUnit * 5
                            Layout.minimumHeight: Kirigami.Units.gridUnit * 5
                            Layout.maximumHeight: Kirigami.Units.gridUnit * 5
                            contentWidth: availableWidth
                            Controls.ScrollBar.horizontal.policy: Controls.ScrollBar.AlwaysOff
                            Controls.ScrollBar.vertical.policy: Controls.ScrollBar.AlwaysOn

                            Controls.TextArea {
                                id: descriptionField

                                width: descriptionView.availableWidth
                                text: root.formDescription
                                placeholderText: "可选，例如：准备礼物、联系某人"
                                wrapMode: TextEdit.Wrap
                                onTextChanged: root.formDescription = text
                            }
                        }

                        RowLayout {
                            Kirigami.FormData.label: "日期"
                            Layout.fillWidth: true
                            spacing: Kirigami.Units.smallSpacing

                            Controls.TextField {
                                id: dateField

                                text: root.formDate
                                placeholderText: "YYYY-MM-DD"
                                onTextEdited: root.formDate = text
                                Layout.fillWidth: true
                            }

                            Controls.Button {
                                text: "今天"
                                onClicked: root.formDate = root.isoDate(new Date())
                            }

                            Controls.Button {
                                text: "明天"
                                onClicked: root.formDate = root.tomorrow()
                            }
                        }

                        RowLayout {
                            Kirigami.FormData.label: "颜色"
                            Layout.fillWidth: true

                            Rectangle {
                                Layout.alignment: Qt.AlignVCenter
                                width: Kirigami.Units.gridUnit
                                height: width
                                radius: width / 2
                                color: root.formColor
                                border.width: 1
                                border.color: Kirigami.Theme.disabledTextColor
                            }

                            Controls.Label {
                                text: root.formColor
                                Layout.fillWidth: true
                            }

                            Controls.Button {
                                icon.name: "color-picker"
                                text: "选择颜色"
                                onClicked: colorDialog.open()
                            }
                        }

                        ColumnLayout {
                            Kirigami.FormData.label: "重复"
                            Kirigami.FormData.labelAlignment: Qt.AlignTop
                            Layout.fillWidth: true
                            spacing: Kirigami.Units.smallSpacing

                            Controls.ComboBox {
                                id: repeatTypeField

                                model: root.repeatTypeLabels
                                currentIndex: root.repeatTypeIndex(root.formRepeatType)
                                onActivated: function(index) {
                                    root.formRepeatType = root.repeatTypeValues[index]
                                }
                                Layout.fillWidth: true
                            }

                            RowLayout {
                                visible: root.formRepeatType === "custom"
                                Layout.fillWidth: true
                                spacing: Kirigami.Units.smallSpacing

                                Controls.Label {
                                    text: "每"
                                }

                                Controls.SpinBox {
                                    id: repeatIntervalField

                                    from: 1
                                    to: 999
                                    value: root.formRepeatInterval
                                    editable: true
                                    onValueModified: root.formRepeatInterval = value
                                    Layout.preferredWidth: Kirigami.Units.gridUnit * 5
                                }

                                Controls.ComboBox {
                                    id: repeatUnitField

                                    model: root.repeatUnitLabels
                                    currentIndex: root.repeatUnitIndex(root.formRepeatUnit)
                                    onActivated: function(index) {
                                        root.formRepeatUnit = root.repeatUnitValues[index]
                                    }
                                    Layout.fillWidth: true
                                }

                                Controls.Label {
                                    text: "重复一次"
                                }
                            }

                            Controls.Label {
                                visible: root.formRepeatType === "monthly"
                                text: "按事件日期的日号重复；没有该日号的月份会跳过。"
                                color: Kirigami.Theme.disabledTextColor
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }
                    }
                }

                Controls.Label {
                    visible: eventsModel.errorString.length > 0
                    text: eventsModel.errorString
                    color: Kirigami.Theme.negativeTextColor
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: Kirigami.Units.smallSpacing

                    Item {
                        Layout.fillWidth: true
                    }

                    Controls.Button {
                        icon.name: "dialog-cancel"
                        text: "取消"
                        onClicked: root.closeEditor()
                    }

                    Controls.Button {
                        icon.name: "document-save"
                        text: root.editingId.length === 0 ? "添加" : "保存"
                        enabled: nameField.text.trim().length > 0 && dateField.text.trim().length > 0
                        onClicked: root.submitForm()
                    }
                }
            }
        }
    }

    ColorDialog {
        id: colorDialog

        title: "选择事件颜色"
        selectedColor: root.formColor
        onAccepted: root.formColor = selectedColor.toString()
    }

    Controls.Dialog {
        id: deleteDialog

        modal: true
        title: "删除事件"
        standardButtons: Controls.Dialog.Yes | Controls.Dialog.No
        // 固定内容宽度，避免 WordWrap 文本与对话框宽度互相影响产生绑定循环。
        implicitWidth: Kirigami.Units.gridUnit * 20
        contentItem: Controls.Label {
            text: "确定删除“" + root.pendingDeleteName + "”吗？"
            wrapMode: Text.WordWrap
            padding: Kirigami.Units.largeSpacing
        }
        onAccepted: eventsModel.removeEvent(root.pendingDeleteId)
    }

    Controls.Dialog {
        id: resetDialog

        modal: true
        title: "恢复系统预置"
        standardButtons: Controls.Dialog.Yes | Controls.Dialog.No
        // 固定内容宽度，避免 WordWrap 文本与对话框宽度互相影响产生绑定循环。
        implicitWidth: Kirigami.Units.gridUnit * 20
        contentItem: Controls.Label {
            text: "这会删除用户事件文件，并重新使用系统预置事件。确定继续吗？"
            wrapMode: Text.WordWrap
            padding: Kirigami.Units.largeSpacing
        }
        onAccepted: eventsModel.resetToSystemDefaults()
    }
}
