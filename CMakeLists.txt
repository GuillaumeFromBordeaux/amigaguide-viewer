# SPDX-FileCopyrightText: 2026 AmigaGuide Viewer contributors
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.20)
project(amigaguide-viewer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# ON  : interface Kirigami (KDE Plasma 6)                     -> qml/Main.qml
# OFF : interface Qt Quick Controls seule (Ubuntu, Linux Mint,
#       GNOME, Cinnamon...) : aucun paquet KDE nécessaire      -> qml/MainQt.qml
option(USE_KIRIGAMI "Interface Kirigami (OFF : Qt Quick Controls seul, sans paquets KDE)" ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Qml Quick Widgets)

add_executable(amigaguide-viewer
    src/main.cpp
    src/amigaguide.cpp
    src/guidebackend.cpp
)

if(USE_KIRIGAMI)
    set(AMIGAGUIDE_QML qml/Main.qml)
    target_compile_definitions(amigaguide-viewer PRIVATE AMIGAGUIDE_KIRIGAMI)
else()
    # MainQt.qml est embarqué sous le nom Main.qml : main.cpp charge toujours qrc:/Main.qml
    set(AMIGAGUIDE_QML qml/MainQt.qml)
    set_source_files_properties(qml/MainQt.qml PROPERTIES QT_RESOURCE_ALIAS Main.qml)
endif()
qt_add_resources(amigaguide-viewer "qml" PREFIX "/" BASE "qml" FILES ${AMIGAGUIDE_QML})

target_link_libraries(amigaguide-viewer PRIVATE
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::Widgets)

include(GNUInstallDirs)
install(TARGETS amigaguide-viewer RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES data/amigaguide-viewer.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES data/amigaguide-viewer-mime.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/mime/packages)

# ---- Paquet .deb (cmake --build build --target package) --------------------
# Prévu pour la variante sans Kirigami : cmake -B build -DUSE_KIRIGAMI=OFF
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "amigaguide-viewer")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Visionneuse de documents AmigaGuide")
set(CPACK_PACKAGE_DESCRIPTION
    "Lecteur de fichiers AmigaGuide (.guide) : navigation par liens, sommaire, recherche plein texte, images @{img} d'AmigaOS 3.2.")
set(CPACK_PACKAGE_CONTACT "AmigaGuide Viewer contributors <noreply@example.invalid>")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/GuillaumeFromBordeaux/amigaguide-viewer")
set(CPACK_DEBIAN_PACKAGE_SECTION "text")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON) # bibliothèques Qt détectées automatiquement
if(NOT USE_KIRIGAMI)
    # Modules QML chargés à l'exécution : invisibles pour l'édition de liens, à lister à la main
    set(CPACK_DEBIAN_PACKAGE_DEPENDS
        "qml6-module-qtquick, qml6-module-qtqml-workerscript, qml6-module-qtquick-window, qml6-module-qtquick-controls, qml6-module-qtquick-templates, qml6-module-qtquick-layouts, qml6-module-qtquick-dialogs, qml6-module-qt-labs-folderlistmodel")
endif()
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "kate")
include(CPack)
