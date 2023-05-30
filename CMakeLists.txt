cmake_minimum_required(VERSION 3.1)

project(vf-gui-translation LANGUAGES CXX)
set(VF_GUI_TRANSLATION_VERSION_MAJOR "1")
set(VF_GUI_TRANSLATION_VERSION_MINOR "3")
set(VF_GUI_TRANSLATION_VERSION_PATCH "0")
set(PROJECT_VERSION "${VF_GUI_TRANSLATION_VERSION_MAJOR}.${VF_GUI_TRANSLATION_VERSION_MINOR}.${VF_GUI_TRANSLATION_VERSION_PATCH}")

include(FeatureSummary)
include(GNUInstallDirs)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 "5.12.0" COMPONENTS Qml Quick CONFIG REQUIRED)

# This is a dummy target introduced to
# * Drop a note once zeratranslation.cpp was changed
# * Add ts-files to qt-creator to make search (& replace) work for ts-files either
add_custom_target(ts_files
    COMMAND echo "zeratranslation.cpp was changed. You might want to run scripts/translate-for-devel.sh"
    DEPENDS ${PROJECT_SOURCE_DIR}/src/lib/zeratranslation.cpp
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/zera-gui_de_DE.ts
        ${CMAKE_CURRENT_SOURCE_DIR}/zera-gui_en_GB.ts
        ${CMAKE_CURRENT_SOURCE_DIR}/zera-gui_en_US.ts
        ${CMAKE_CURRENT_SOURCE_DIR}/zera-gui_fr_FR.ts
)

add_subdirectory(src)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

