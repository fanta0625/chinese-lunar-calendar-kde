import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.plasma.private.locallunarcalendar 1.0

Item {
    id: root

    implicitWidth: 560
    implicitHeight: 500

    property string editingId: ""
    property string formName: ""
    property string formDate: ""
    property string formColor: "#8e24aa"
    property bool formRepeatYearly: false
    property string pendingDeleteId: ""
    property string pendingDeleteName: ""

    function isoDate(date) {
        return Qt.formatDate(date, "yyyy-MM-dd")
    }

    function openAdd() {
        editingId = ""
        formName = ""
        formDate = isoDate(new Date())
        formColor = "#8e24aa"
        formRepeatYearly = false
        eventsModel.clearError()
        editorSheet.open()
    }

    function openEdit(eventId, eventName, eventDate, eventColor, repeatYearly) {
        editingId = eventId
        formName = eventName
        formDate = eventDate
        formColor = eventColor
        formRepeatYearly = repeatYearly
        eventsModel.clearError()
        editorSheet.open()
    }

    function tomorrow() {
        var date = new Date()
        date.setDate(date.getDate() + 1)
        return isoDate(date)
    }

    function submitForm() {
        eventsModel.clearError()
        var saved
        if (editingId.length === 0) {
            saved = eventsModel.addEvent(formName, formDate, formColor, formRepeatYearly)
        } else {
            saved = eventsModel.updateEvent(editingId, formName, formDate, formColor, formRepeatYearly)
        }
        if (saved) {
            editorSheet.close()
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
        anchors.fill: parent
        anchors.margins: Kirigami.Units.largeSpacing
        spacing: Kirigami.Units.largeSpacing

        Controls.Label {
            text: "自定义事件"
            font.bold: true
            Layout.fillWidth: true
        }

        Controls.Label {
            text: eventsModel.usingSystemDefaults
                  ? "当前使用系统预置事件；第一次编辑时会复制到用户文件。"
                  : "事件保存在用户文件中。"
            color: Kirigami.Theme.disabledTextColor
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        ListView {
            id: eventList

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 220
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
                            text: model.date + (model.repeatYearly ? " · 每年重复" : "")
                            color: Kirigami.Theme.disabledTextColor
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }

                    Controls.ToolButton {
                        icon.name: "document-edit"
                        display: Controls.AbstractButton.IconOnly
                        onClicked: root.openEdit(model.id, model.name, model.date, model.color, model.repeatYearly)
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

                onClicked: root.openEdit(model.id, model.name, model.date, model.color, model.repeatYearly)
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

    Kirigami.OverlaySheet {
        id: editorSheet

        parent: root
        width: Math.min(root.width, Kirigami.Units.gridUnit * 30)
        header: Kirigami.Heading {
            text: root.editingId.length === 0 ? "添加事件" : "编辑事件"
            level: 2
        }

        ColumnLayout {
            width: editorSheet.width - Kirigami.Units.largeSpacing * 2
            spacing: Kirigami.Units.smallSpacing

            Controls.Label {
                text: "名称"
                Layout.fillWidth: true
            }

            Controls.TextField {
                id: nameField

                text: root.formName
                placeholderText: "例如：爸爸生日"
                onTextEdited: root.formName = text
                Layout.fillWidth: true
            }

            Controls.Label {
                text: "日期"
                Layout.fillWidth: true
            }

            RowLayout {
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

            Controls.Label {
                text: "颜色"
                Layout.fillWidth: true
            }

            RowLayout {
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

            Controls.CheckBox {
                text: "每年重复"
                checked: root.formRepeatYearly
                onToggled: root.formRepeatYearly = checked
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
                Layout.topMargin: Kirigami.Units.smallSpacing

                Item {
                    Layout.fillWidth: true
                }

                Controls.Button {
                    text: "取消"
                    onClicked: editorSheet.close()
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
        contentItem: Controls.Label {
            text: "这会删除用户事件文件，并重新使用系统预置事件。确定继续吗？"
            wrapMode: Text.WordWrap
            padding: Kirigami.Units.largeSpacing
        }
        onAccepted: eventsModel.resetToSystemDefaults()
    }
}
