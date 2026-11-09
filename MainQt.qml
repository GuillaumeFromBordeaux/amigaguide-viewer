// SPDX-FileCopyrightText: 2026 AmigaGuide Viewer contributors
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Dialogs
import QtQuick.Layouts

// Variante de l'interface SANS Kirigami : elle n'utilise que Qt Quick Controls 2.
// Elle fonctionne avec les seuls paquets Qt 6 d'Ubuntu 24.04 / Linux Mint 22.
// Même disposition que Main.qml : barre d'outils pleine largeur, SplitView dessous.
QQC2.ApplicationWindow {
    id: root

    // Unités dérivées de la police du système (équivalent de Kirigami.Units)
    readonly property real gu: fm.height
    readonly property real smallSpacing: Math.round(gu / 4)
    readonly property real largeSpacing: Math.round(gu / 2)

    visible: true
    width: 1000
    height: 720
    minimumWidth: 700
    title: guide.loaded ? guide.guideTitle + " — " + guide.nodeTitle : "AmigaGuide Viewer"

    FontMetrics {
        id: fm
    }

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
            msgText.text = text;
            msg.visible = true;
        }

        function onStateChanged() {
            msg.visible = false;
            scroll.contentItem.contentY = 0;
        }

        target: guide
    }

    // ---- Fenêtre de recherche ---------------------------------------------
    QQC2.Dialog {
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

        parent: QQC2.Overlay.overlay
        anchors.centerIn: parent
        modal: true
        title: "Rechercher dans le guide"
        width: Math.min(root.width - root.gu * 2, root.gu * 32)
        height: Math.min(root.height - root.gu * 2, root.gu * 26)
        footer: QQC2.DialogButtonBox {
            QQC2.Button {
                text: "Fermer"
                QQC2.DialogButtonBox.buttonRole: QQC2.DialogButtonBox.RejectRole
            }
        }
        onOpened: {
            searchField.forceActiveFocus();
            searchField.selectAll();
        }

        contentItem: Item {
            QQC2.TextField {
                id: searchField

                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                placeholderText: "Rechercher…"
                onTextChanged: searchDialog.runSearch()
                Keys.onReturnPressed: searchDialog.openFirstResult()
                Keys.onEnterPressed: searchDialog.openFirstResult()
            }

            QQC2.Label {
                id: resultCount

                anchors.top: searchField.bottom
                anchors.topMargin: root.smallSpacing
                anchors.left: parent.left
                anchors.right: parent.right
                height: root.gu * 1.6 // hauteur fixe : évite une boucle de liaison
                verticalAlignment: Text.AlignVCenter
                visible: searchField.text.trim().length >= 2
                opacity: 0.7
                text: searchDialog.results.length === 0 ? "Aucun résultat" : searchDialog.results.length + " nœud(s) trouvé(s)"
            }

            ListView {
                id: resultList

                anchors.top: resultCount.bottom
                anchors.topMargin: root.smallSpacing
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
                            font.pixelSize: Math.round(root.gu * 0.8)
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

    // ---- Barre d'outils pleine largeur -------------------------------------
    header: QQC2.ToolBar {
        contentItem: RowLayout {
            spacing: root.smallSpacing

            Item {
                Layout.fillWidth: true
            }

            QQC2.ToolButton {
                text: "Retour"
                icon.name: "go-previous-view"
                enabled: guide.canBack
                onClicked: guide.goBack()
            }

            QQC2.ToolButton {
                text: "Précédent"
                icon.name: "go-previous"
                enabled: guide.canPrev
                onClicked: guide.goPrev()
            }

            QQC2.ToolButton {
                text: "Suivant"
                icon.name: "go-next"
                enabled: guide.canNext
                onClicked: guide.goNext()
            }

            QQC2.ToolButton {
                text: "Sommaire"
                icon.name: "go-home"
                enabled: guide.canContents
                onClicked: guide.goContents()
            }

            QQC2.ToolButton {
                text: "Index"
                icon.name: "view-list-text"
                enabled: guide.hasIndex
                onClicked: guide.goIndex()
            }

            QQC2.ToolButton {
                text: "Aide"
                icon.name: "help-contents"
                enabled: guide.hasHelp
                onClicked: guide.goHelp()
            }
        }
    }

    // ---- Volet latéral + document ------------------------------------------
    QQC2.SplitView {
        id: mainSplit

        anchors.fill: parent
        orientation: Qt.Horizontal

        Rectangle {
            id: sidePanel

            QQC2.SplitView.preferredWidth: 260
            QQC2.SplitView.minimumWidth: 160
            QQC2.SplitView.maximumWidth: 500
            QQC2.SplitView.fillHeight: true
            color: root.palette.base

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

                // Ligne séparatrice grise sous « Éditer dans Kate »
                Rectangle {
                    Layout.fillWidth: true
                    Layout.topMargin: root.smallSpacing
                    Layout.bottomMargin: root.smallSpacing
                    implicitHeight: 1
                    color: root.palette.mid
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

            // Bandeau d'avertissement (remplace Kirigami.InlineMessage)
            Rectangle {
                id: msg

                anchors.top: parent.top
                width: parent.width
                height: msgText.implicitHeight + root.gu
                visible: false
                z: 2
                color: "#fff3cd"
                border.color: "#e0c36a"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: root.largeSpacing
                    anchors.rightMargin: root.smallSpacing

                    QQC2.Label {
                        id: msgText

                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        color: "#5c4400"
                    }

                    QQC2.ToolButton {
                        text: "✕"
                        onClicked: msg.visible = false
                    }
                }
            }

            // Écran d'accueil quand aucun guide n'est ouvert
            ColumnLayout {
                anchors.centerIn: parent
                visible: !guide.loaded
                spacing: root.largeSpacing

                QQC2.Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: "Aucun guide ouvert"
                    font.pixelSize: Math.round(root.gu * 1.4)
                    opacity: 0.7
                }

                QQC2.Button {
                    Layout.alignment: Qt.AlignHCenter
                    text: "Ouvrir un fichier .guide…"
                    icon.name: "document-open"
                    onClicked: fileDialog.open()
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
                    padding: root.largeSpacing * 2
                    textFormat: Text.RichText
                    text: guide.html
                    font: fixedFont // police à chasse fixe du système (fournie par main.cpp)
                    wrapMode: guide.wrap ? Text.Wrap : Text.NoWrap
                    linkColor: root.palette.link
                    onLinkActivated: (link) => guide.followLink(link)

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
            color: QQC2.SplitHandle.pressed ? root.palette.highlight : (QQC2.SplitHandle.hovered ? root.palette.mid : root.palette.midlight)
        }
    }
}
