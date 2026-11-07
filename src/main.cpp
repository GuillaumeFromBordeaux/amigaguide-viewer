// SPDX-FileCopyrightText: 2026 AmigaGuide Viewer contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>

#include "guidebackend.h"

int main(int argc, char *argv[])
{
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE"))
        qputenv("QT_QUICK_CONTROLS_STYLE", "org.kde.desktop");

    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("AmigaGuide Viewer"));
    QApplication::setDesktopFileName(QStringLiteral("amigaguide-viewer"));

    GuideBackend backend;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("guide"), &backend);
    engine.load(QUrl(QStringLiteral("qrc:/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return 1;

    const QStringList args = app.arguments();
    if (args.size() > 1)
        backend.openPath(args.at(1));

    return app.exec();
}
