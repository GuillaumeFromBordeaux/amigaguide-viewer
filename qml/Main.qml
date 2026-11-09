// SPDX-FileCopyrightText: 2026 AmigaGuide Viewer contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Dialogs
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// Barre d'outils sur toute la largeur, SplitView redimensionnable en dessous.
// Le panneau est un Item ordinaire : Kirigami ne réserve plus de largeur au GlobalDrawer.
Kirigami.ApplicationWindow {
    id: root

    width: 1000
    height: 720
    minimumWidth: 500
    title: guide.loaded ? guide.guideTitle + " — " + guide.nodeTitle : "AmigaGuide Viewer"
    pageStack.globalToolBar.style: Kirigami.ApplicationHeaderStyle.None

    Shortcut {
        sequence: StandardKey.Open
        onActivated: fileDialog.open()
    }

    Shortcut {
        sequence: StandardKey.Find
        enabled: guide.loaded
        onActivated: searchDialog.open()
    }

    Shortcut {
        sequence: "Ctrl+E"
        enabled: guide.loaded
        onActivated: guide.editInKate()
    }

    Shortcut {
        sequence: "Alt+Left"
        enabled: guide.canBack
        onActivated: guide.goBack()
    }

    Shortcut {
        sequence: "Alt+Home"
        enabled: guide.canContents
        onActivated: guide.goContents()
    }

    Shortcut {
        sequence: "F1"
        enabled: guide.hasHelp
        onActivated: guide.goHelp()
    }

    FileDialog {
        id: fileDialog

        title: "Ouvrir un guide AmigaGuide"
        nameFilters: ["AmigaGuide (*.guide *.GUIDE *.amigaguide)", "Tous les fichiers (*)"]
        onAccepted: guide.openUrl(selectedFile)
    }

    Connections {
        function onMessage(text) {
            msg.text = text;
            msg.visible = true;
        }

        function onStateChanged() {
            msg.visible = false;
            scroll.contentItem.contentY = 0;
        }

        target: guide
    }

    Kirigami.Dialog {
        id: searchDialog

        property var results: []

        function openFirstResult() {
            if (results.length > 0) {
                guide.gotoNode(results[0].key);
                close();
            }
        }

        function runSearch() {
            results = searchField.text.trim().length >= 2 ? guide.search(searchField.text) : [];
        }

        title: "Rechercher dans le guide"
        standardButtons: Kirigami.Dialog.NoButton
        padding: Kirigami.Units.largeSpacing
        onOpened: {
            searchField.forceActiveFocus();
            searchField.selectAll();
        }

        Item {
            // Tailles explicites + ancres : la liste ne peut plus être écrasée à une hauteur nulle
            implicitWidth: Kirigami.Units.gridUnit * 30
            implicitHeight: Kirigami.Units.gridUnit * 22

            Kirigami.SearchField {
                id: searchField

                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                onTextChanged: searchDialog.runSearch()
                Keys.onReturnPressed: searchDialog.openFirstResult()
                Keys.onEnterPressed: searchDialog.openFirstResult()
            }

            QQC2.Label {
                id: resultCount

                anchors.top: searchField.bottom
                anchors.topMargin: Kirigami.Units.smallSpacing
                anchors.left: parent.left
                anchors.right: parent.right
                height: visible ? implicitHeight : 0
                visible: searchField.text.trim().length >= 2
                opacity: 0.7
                text: searchDialog.results.length === 0 ? "Aucun résultat" : searchDialog.results.length + " nœud(s) trouvé(s)"
            }

            ListView {
                id: resultList

                anchors.top: resultCount.bottom
                anchors.topMargin: Kirigami.Units.smallSpacing
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                clip: true
                model: searchDialog.results

                delegate: QQC2.ItemDelegate {
                    required property var modelData

                    width: ListView.view.width
                    onClicked: {
                        guide.gotoNode(modelData.key);
                        searchDialog.close();
                    }

                    contentItem: ColumnLayout {
                        spacing: 0

                        QQC2.Label {
                            Layout.fillWidth: true
                            text: modelData.title
                            textFormat: Text.PlainText
                            font.bold: true
                            elide: Text.ElideRight
                        }

                        QQC2.Label {
                            Layout.fillWidth: true
                            visible: modelData.snippet.length > 0
                            text: modelData.snippet
                            textFormat: Text.PlainText
                            font: Kirigami.Theme.smallFont
                            opacity: 0.7
                            elide: Text.ElideRight
                        }
                    }
                }

                QQC2.ScrollBar.vertical: QQC2.ScrollBar {
                }
            }
        }
    }

    pageStack.initialPage: Kirigami.Page {
        id: mainPage

        padding: 0
        title: ""

        QQC2.SplitView {
            id: mainSplit

            anchors.fill: parent
            orientation: Qt.Horizontal

            // Volet latéral gauche
            Rectangle {
                id: sidePanel

                QQC2.SplitView.preferredWidth: 260
                QQC2.SplitView.minimumWidth: 160
                QQC2.SplitView.maximumWidth: 500
                QQC2.SplitView.fillHeight: true
                // Fond de type « zone de contenu » : blanc en thème clair,
                // sombre en thème sombre (le texte s'adapte automatiquement).
                Kirigami.Theme.colorSet: Kirigami.Theme.View
                Kirigami.Theme.inherit: false
                color: Kirigami.Theme.backgroundColor

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    QQC2.ItemDelegate {
                        Layout.fillWidth: true
                        text: "Ouvrir…"
                        icon.name: "document-open"
                        onClicked: fileDialog.open()
                    }

                    QQC2.ItemDelegate {
                        Layout.fillWidth: true
                        text: "Rechercher…"
                        icon.name: "edit-find"
                        enabled: guide.loaded
                        onClicked: searchDialog.open()
                    }

                    QQC2.ItemDelegate {
                        Layout.fillWidth: true
                        text: "Éditer dans Kate"
                        icon.name: "document-edit"
                        enabled: guide.loaded
                        onClicked: guide.editInKate()
                    }

                    Kirigami.Separator {
                        Layout.fillWidth: true
                        Layout.topMargin: Kirigami.Units.smallSpacing
                        Layout.bottomMargin: Kirigami.Units.smallSpacing
                    }

                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: guide.contents

                        delegate: QQC2.ItemDelegate {
                            required property var modelData

                            width: ListView.view.width
                            text: modelData.label
                            highlighted: modelData.key !== "" && modelData.key === guide.currentKey
                            onClicked: guide.followLink(modelData.href)
                        }

                        QQC2.ScrollBar.vertical: QQC2.ScrollBar {
                        }

                    }

                }

            }

            Item {
                QQC2.SplitView.fillWidth: true
                QQC2.SplitView.fillHeight: true
                QQC2.SplitView.minimumWidth: 240

                Kirigami.InlineMessage {
                    id: msg

                    position: Kirigami.InlineMessage.Position.Header
                    type: Kirigami.MessageType.Warning
                    showCloseButton: true
                    visible: false
                    anchors.top: parent.top
                    width: parent.width
                    z: 2
                }

                Kirigami.PlaceholderMessage {
                    anchors.centerIn: parent
                    width: Math.max(0, parent.width - Kirigami.Units.gridUnit * 4)
                    visible: !guide.loaded
                    icon.name: "document-preview-archive"
                    text: "Aucun guide ouvert"

                    helpfulAction: Kirigami.Action {
                        text: "Ouvrir un fichier .guide…"
                        icon.name: "document-open"
                        onTriggered: fileDialog.open()
                    }

                }

                QQC2.ScrollView {
                    id: scroll

                    anchors.fill: parent
                    visible: guide.loaded
                    contentWidth: guide.wrap ? availableWidth : label.implicitWidth

                    QQC2.Label {
                        id: label

                        width: guide.wrap ? scroll.availableWidth : implicitWidth
                        padding: Kirigami.Units.largeSpacing * 2
                        textFormat: Text.RichText
                        text: guide.html
                        font: Kirigami.Theme.fixedWidthFont
                        wrapMode: guide.wrap ? Text.Wrap : Text.NoWrap
                        linkColor: Kirigami.Theme.linkColor
                        onLinkActivated: (link) => {
                            return guide.followLink(link);
                        }

                        HoverHandler {
                            cursorShape: label.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                        }

                    }

                }

            }

            // Le séparateur peut être déplacé à la souris.
            handle: Rectangle {
                implicitWidth: 5
                implicitHeight: 5
                color: QQC2.SplitHandle.pressed ? Kirigami.Theme.highlightColor : (QQC2.SplitHandle.hovered ? Kirigami.Theme.disabledTextColor : Kirigami.Theme.alternateBackgroundColor)
            }

        }

        // En-tête de la page : sa largeur est celle de la fenêtre entière.
        header: QQC2.ToolBar {
            id: mainToolBar

            contentItem: RowLayout {
                spacing: Kirigami.Units.smallSpacing

                Kirigami.ActionToolBar {
                    Layout.fillWidth: true
                    alignment: Qt.AlignRight
                    actions: [
                        Kirigami.Action {
                            text: "Retour"
                            icon.name: "go-previous-view"
                            enabled: guide.canBack
                            onTriggered: guide.goBack()
                        },
                        Kirigami.Action {
                            text: "Précédent"
                            icon.name: "go-previous"
                            enabled: guide.canPrev
                            onTriggered: guide.goPrev()
                        },
                        Kirigami.Action {
                            text: "Suivant"
                            icon.name: "go-next"
                            enabled: guide.canNext
                            onTriggered: guide.goNext()
                        },
                        Kirigami.Action {
                            text: "Sommaire"
                            icon.name: "go-home"
                            enabled: guide.canContents
                            onTriggered: guide.goContents()
                        },
                        Kirigami.Action {
                            text: "Index"
                            icon.name: "view-list-text"
                            enabled: guide.hasIndex
                            onTriggered: guide.goIndex()
                        },
                        Kirigami.Action {
                            text: "Aide"
                            icon.name: "document-preview-archive"
                            enabled: guide.hasHelp
                            onTriggered: guide.goHelp()
                        }
                    ]
                }

            }

        }

    }

}
