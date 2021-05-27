find_package(Qt5QuickCompiler CONFIG REQUIRED)
if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    qt5_add_resources(PLUGIN_RESOURCES ${CMAKE_CURRENT_LIST_DIR}/zeratranslation.qrc)
else()
    qtquick_compiler_add_resources(PLUGIN_RESOURCES ${CMAKE_CURRENT_LIST_DIR}/zeratranslation.qrc)
endif()

# lib target
add_library(ZeraTranslation
    SHARED
        # headers for moc & qtcreator to have it in-tree
        ${PROJECT_SOURCE_DIR}/include/zeratranslation.h
        ${PROJECT_SOURCE_DIR}/include/zeratranslationplugin.h
        # sources
        zeratranslation.cpp
        zeratranslationplugin.cpp
        ${PLUGIN_RESOURCES}
)
add_library(ZeraTranslation::ZeraTranslation ALIAS ZeraTranslation)
set_target_properties(ZeraTranslation PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(ZeraTranslation PROPERTIES SOVERSION ${VF_GUI_TRANSLATION_VERSION_MAJOR})

# export header
include(GenerateExportHeader)
generate_export_header(ZeraTranslation)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/zeratranslation_export.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ZeraTranslation
)


# where to find headers
target_include_directories(ZeraTranslation
    PUBLIC # consumer
        $<INSTALL_INTERFACE:include/ZeraTranslation>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        #generated export header
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

# additional compiler options
target_compile_options(ZeraTranslation PRIVATE -Wall)

# add Qml-Debug for Bebug build envirionemt only
target_compile_definitions(ZeraTranslation PRIVATE $<$<CONFIG:Debug>:QT_QML_DEBUG>)

target_link_libraries(ZeraTranslation
    PRIVATE
        Qt5::Core
        Qt5::Qml
        # ask linker to help us finding unresolved symbols
        "-Wl,--no-undefined"
)

# install public header
install(
    FILES
        ${PROJECT_SOURCE_DIR}/include/zeratranslation.h
        ${PROJECT_SOURCE_DIR}/include/zeratranslationplugin.h
    DESTINATION
        ${CMAKE_INSTALL_INCLUDEDIR}/ZeraTranslation
)

# install library
install(TARGETS ZeraTranslation
    EXPORT ZeraTranslationExport
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# configure ZeraTranslationConfigVersion.cmake
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ZeraTranslationConfigVersion.cmake
    VERSION ${PACKAGE_VERSION}
    COMPATIBILITY SameMinorVersion
)

# configure ZeraTranslationConfig.cmake
configure_file(ZeraTranslationConfig.cmake.in ZeraTranslationConfig.cmake @ONLY)

# install ZeraTranslationConfig(Version).cmake
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/ZeraTranslationConfig.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/ZeraTranslationConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ZeraTranslation
)

# install targets cmake-files
install(EXPORT ZeraTranslationExport
    FILE ZeraTranslationTargets.cmake
    NAMESPACE ZeraTranslation::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ZeraTranslation
)

