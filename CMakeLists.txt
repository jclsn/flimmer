cmake_minimum_required(VERSION 3.21)
project(plasma-tv VERSION 0.1.0 LANGUAGES CXX)

# Keep debug symbols by default so crash backtraces are useful.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets Quick QuickControls2 Network Sql Multimedia DBus)
find_package(Qt6Keychain REQUIRED)

qt_standard_project_setup(REQUIRES 6.5)
qt_policy(SET QTP0004 NEW)

qt_add_executable(plasma-tv src/main.cpp)

qt_add_qml_module(plasma-tv
    URI PlasmaTv
    VERSION 1.0
    # Shared Qt build: QML plugins (incl. Kirigami) are loaded at runtime.
    NO_IMPORT_SCAN
    SOURCES
        src/serverconfig.h src/serverconfig.cpp
        src/colorschemes.h src/colorschemes.cpp
        src/networkauth.h src/networkauth.cpp
        src/appsettings.h src/appsettings.cpp
        src/tvhapi.h src/tvhapi.cpp
        src/epgdatabase.h src/epgdatabase.cpp
        src/epgstore.h src/epgstore.cpp
        src/channelmodel.h src/channelmodel.cpp
        src/tvhbackend.h src/tvhbackend.cpp
        src/screensaverinhibitor.h src/screensaverinhibitor.cpp
)

target_include_directories(plasma-tv PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(plasma-tv PRIVATE
    Qt6::Widgets Qt6::Quick Qt6::QuickControls2 Qt6::Network Qt6::Sql Qt6::Multimedia Qt6::DBus
    Qt6Keychain::Qt6Keychain)

include(GNUInstallDirs)
# Kirigami and the Plasma Qt Quick Controls style are only used from QML, so they
# are runtime dependencies (Ubuntu: qml6-module-org-kde-kirigami, qml6-module-org-kde-desktop,
# kde-style-breeze; Arch: kirigami, qqc2-desktop-style, breeze).

install(TARGETS plasma-tv RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES data/plasma-tv.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
