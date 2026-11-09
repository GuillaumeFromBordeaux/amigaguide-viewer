// SPDX-FileCopyrightText: 2026 AmigaGuide Viewer contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QApplication>
#include <QFontDatabase>
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QUrl>

#include "guidebackend.h"

int main(int argc, char *argv[])
{
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
#ifdef AMIGAGUIDE_KIRIGAMI
        qputenv("QT_QUICK_CONTROLS_STYLE", "org.kde.desktop");
#else
        qputenv("QT_QUICK_CONTROLS_STYLE", "Fusion"); // suit la palette du système, sans paquet KDE
#endif
    }

    QApplication app(argc, argv);

    // Traductions de Qt installées sur le système (paquet qt6-translations-l10n sous Ubuntu)
    QTranslator qtBaseTr, qtDeclarativeTr;
    const QString trDir = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
    if (qtBaseTr.load(QLocale(), QStringLiteral("qtbase"), QStringLiteral("_"), trDir))
        app.installTranslator(&qtBaseTr);
    if (qtDeclarativeTr.load(QLocale(), QStringLiteral("qtdeclarative"), QStringLiteral("_"), trDir))
        app.installTranslator(&qtDeclarativeTr);

    QApplication::setApplicationName(QStringLiteral("AmigaGuide Viewer"));
    QApplication::setDesktopFileName(QStringLiteral("amigaguide-viewer"));

    GuideBackend backend;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("guide"), &backend);
    // Police à chasse fixe du système (utilisée par la variante sans Kirigami)
    engine.rootContext()->setContextProperty(QStringLiteral("fixedFont"),
                                             QFontDatabase::systemFont(QFontDatabase::FixedFont));
    engine.load(QUrl(QStringLiteral("qrc:/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return 1;

    const QStringList args = app.arguments();
    if (args.size() > 1)
        backend.openPath(args.at(1));

    return app.exec();
}
