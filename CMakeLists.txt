cmake_minimum_required(VERSION 3.1)

# project definition / version
project(vf-gui-translation LANGUAGES CXX)
set(VF_GUI_TRANSLATION_VERSION_MAJOR "0")
set(VF_GUI_TRANSLATION_VERSION_MINOR "1")
set(VF_GUI_TRANSLATION_VERSION_PATCH "0")
set(PROJECT_VERSION "${VF_GUI_TRANSLATION_VERSION_MAJOR}.${VF_GUI_TRANSLATION_VERSION_MINOR}.${VF_GUI_TRANSLATION_VERSION_PATCH}")

# cmake macros
include(FeatureSummary)
include(GNUInstallDirs)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# For Qt's sake
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Qt5 libs
find_package(Qt5 "5.12.0" COMPONENTS Qml Quick CONFIG REQUIRED)

add_subdirectory(src)

# spawn out some info on configuration
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

