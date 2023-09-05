cmake_minimum_required(VERSION 3.5)

project(vf-gui-translation LANGUAGES CXX)
set(VF_GUI_TRANSLATION_VERSION_MAJOR "1")
set(VF_GUI_TRANSLATION_VERSION_MINOR "3")
set(VF_GUI_TRANSLATION_VERSION_PATCH "0")
set(PROJECT_VERSION "${VF_GUI_TRANSLATION_VERSION_MAJOR}.${VF_GUI_TRANSLATION_VERSION_MINOR}.${VF_GUI_TRANSLATION_VERSION_PATCH}")

include(FeatureSummary)
include(GNUInstallDirs)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 "5.14.0" COMPONENTS Qml Quick LinguistTools CONFIG REQUIRED)
find_program(LRELEASE NAMES lrelease lrelease-qt5 NO_CACHE REQUIRED)
find_program(LUPDATE NAMES lupdate lupdate-qt5 NO_CACHE REQUIRED)

file(GLOB TS_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.ts
    )

add_custom_target(ts_files ALL
    DEPENDS ${TS_FILES} flag_copy.sh
    SOURCES ${TS_FILES} flag_copy.sh
    COMMAND ${LUPDATE} -no-obsolete -locations none -no-ui-lines ${CMAKE_CURRENT_SOURCE_DIR}/src/lib/zeratranslation.cpp -ts ${TS_FILES}
    COMMAND ${LRELEASE} ${TS_FILES}
    COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/translations
    COMMAND mv ${CMAKE_CURRENT_SOURCE_DIR}/*.qm ${CMAKE_CURRENT_BINARY_DIR}/translations
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/flag_copy.sh ${CMAKE_CURRENT_SOURCE_DIR}/region-flags ${CMAKE_CURRENT_BINARY_DIR}/translations
)

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/translations
    DESTINATION ${CMAKE_INSTALL_DATADIR}/zera
)
add_definitions(-DZERA_TRANSLATION_PATH="${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/zera/translations")

add_subdirectory(src)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
