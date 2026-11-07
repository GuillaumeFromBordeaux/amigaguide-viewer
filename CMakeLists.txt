# SPDX-FileCopyrightText: 2026 AmigaGuide Viewer contributors
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.20)
project(amigaguide-viewer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Qml Quick Widgets)

add_executable(amigaguide-viewer
    src/main.cpp
    src/amigaguide.cpp
    src/guidebackend.cpp
)
qt_add_resources(amigaguide-viewer "qml" PREFIX "/" BASE "qml" FILES qml/Main.qml)

target_link_libraries(amigaguide-viewer PRIVATE
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::Widgets)

include(GNUInstallDirs)
install(TARGETS amigaguide-viewer RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES data/amigaguide-viewer.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES data/amigaguide-viewer-mime.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/mime/packages)
