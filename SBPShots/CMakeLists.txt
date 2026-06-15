# ======================================================================================================================
#   SBP Shots - Screenshot automation tool for sub-bottom profiler monitoring during oceanographic surveys.
#   Copyright (C) 2022-2026 Angel Vera Herrera <avera@roa.es> - Real Instituto y Observatorio de la Armada (ROA)
#
#   This file is part of SBP Shots.
#
#   SBP Shots is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
#   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any
#   later version.
#
#   SBP Shots is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
#   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
#   details.
#
#   You should have received a copy of the GNU General Public License along with SBP Shots.
#   If not, see <http://www.gnu.org/licenses/>.
# ======================================================================================================================

cmake_minimum_required(VERSION 3.24)

# ----------------------------------------------------------------------------------------------------------------------
# PROJECT CONFIGURATION
# ----------------------------------------------------------------------------------------------------------------------

set(PROJECT_TARGET_NAME SBPShots)
set(PROJECT_DISPLAY_NAME "SBP Shots")
set(PROJECT_VERSION_VALUE 2606.3)

project(${PROJECT_TARGET_NAME}
    VERSION ${PROJECT_VERSION_VALUE}
    DESCRIPTION "Screenshot automation tool for sub-bottom profiler monitoring during oceanographic surveys."
    LANGUAGES CXX
)

# ----------------------------------------------------------------------------------------------------------------------
# GLOBAL CMAKE / QT CONFIGURATION
# ----------------------------------------------------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Keep generated ui_*.h accessible to sources.
set(CMAKE_AUTOUIC_SEARCH_PATHS
    ${CMAKE_CURRENT_SOURCE_DIR}/forms
)

# Better folder organization in Visual Studio / Qt Creator.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Runtime output directory for build tree.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ----------------------------------------------------------------------------------------------------------------------
# INSTALL LAYOUT
# ----------------------------------------------------------------------------------------------------------------------

include(GNUInstallDirs)

# On Windows, keep the final deployed directory flat:
#
#   SBPShots.exe
#   Qt6Core.dll
#   Qt6Gui.dll
#   Qt6Widgets.dll
#   platforms/qwindows.dll
#
# This must be set before find_package(Qt6), because Qt initializes its deploy
# runtime directory from CMAKE_INSTALL_BINDIR.
if(WIN32)
    set(CMAKE_INSTALL_BINDIR "." CACHE PATH "Runtime install directory" FORCE)
endif()

# ----------------------------------------------------------------------------------------------------------------------
# QT DEPENDENCIES
# ----------------------------------------------------------------------------------------------------------------------

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
)

qt_standard_project_setup()

# ----------------------------------------------------------------------------------------------------------------------
# PROJECT DIRECTORIES
# ----------------------------------------------------------------------------------------------------------------------

set(PROJECT_INCLUDES_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/includes)
set(PROJECT_SOURCES_DIR   ${CMAKE_CURRENT_SOURCE_DIR}/sources)
set(PROJECT_FORMS_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/forms)
set(PROJECT_RESOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources)
set(PROJECT_DOCS_DIR      ${CMAKE_CURRENT_SOURCE_DIR}/docs)
set(PROJECT_TOOLS_DIR     ${CMAKE_CURRENT_SOURCE_DIR}/tools)
set(PROJECT_SCRIPTS_DIR   ${CMAKE_CURRENT_SOURCE_DIR}/scripts)

# ----------------------------------------------------------------------------------------------------------------------
# AUTO FILE COLLECTION
# ----------------------------------------------------------------------------------------------------------------------

file(GLOB_RECURSE PROJECT_HEADERS CONFIGURE_DEPENDS
    ${PROJECT_INCLUDES_DIR}/*.h
    ${PROJECT_INCLUDES_DIR}/*.hpp
)

file(GLOB_RECURSE PROJECT_INLINE CONFIGURE_DEPENDS
    ${PROJECT_INCLUDES_DIR}/*.inl
    ${PROJECT_INCLUDES_DIR}/*.tpp
)

file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
    ${PROJECT_SOURCES_DIR}/*.cpp
    ${PROJECT_SOURCES_DIR}/*.cxx
    ${PROJECT_SOURCES_DIR}/*.cc
)

file(GLOB_RECURSE PROJECT_FORMS CONFIGURE_DEPENDS
    ${PROJECT_FORMS_DIR}/*.ui
)

file(GLOB_RECURSE PROJECT_RESOURCES CONFIGURE_DEPENDS
    ${PROJECT_RESOURCES_DIR}/*.qrc
)

file(GLOB_RECURSE PROJECT_DOCS CONFIGURE_DEPENDS
    ${PROJECT_DOCS_DIR}/*.md
    ${PROJECT_DOCS_DIR}/*.txt
)

file(GLOB_RECURSE PROJECT_SCRIPTS CONFIGURE_DEPENDS
    ${PROJECT_SCRIPTS_DIR}/*.py
    ${PROJECT_SCRIPTS_DIR}/*.bat
    ${PROJECT_SCRIPTS_DIR}/*.ps1
)

set(PROJECT_WINDOWS_RESOURCES)

if(WIN32)
    # Collect any static .rc files (templates *.rc.in are handled separately below).
    file(GLOB_RECURSE PROJECT_WINDOWS_RESOURCES CONFIGURE_DEPENDS
        ${PROJECT_RESOURCES_DIR}/*.rc
    )

    # Generate the versioned application resource (icon + VERSIONINFO) from a template, so the version embedded in
    # the executable file properties always matches the CMake project version. A two-component version such as
    # 2606.3 leaves PROJECT_VERSION_PATCH empty, so fall back to 0 for the numeric FILEVERSION/PRODUCTVERSION fields.
    set(SBPSHOTS_RC_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
    set(SBPSHOTS_RC_VERSION_MINOR ${PROJECT_VERSION_MINOR})

    if(NOT DEFINED PROJECT_VERSION_PATCH OR PROJECT_VERSION_PATCH STREQUAL "")
        set(SBPSHOTS_RC_VERSION_PATCH 0)
    else()
        set(SBPSHOTS_RC_VERSION_PATCH ${PROJECT_VERSION_PATCH})
    endif()

    set(SBPSHOTS_RC_ICON_PATH "${PROJECT_RESOURCES_DIR}/icons/icon.ico")

    set(SBPSHOTS_GENERATED_RC ${CMAKE_CURRENT_BINARY_DIR}/sbpshots_version.rc)
    configure_file(
        ${PROJECT_RESOURCES_DIR}/windows/app_icon.rc.in
        ${SBPSHOTS_GENERATED_RC}
        @ONLY
    )

    list(APPEND PROJECT_WINDOWS_RESOURCES ${SBPSHOTS_GENERATED_RC})
endif()

set(PROJECT_LICENSE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE)

# ----------------------------------------------------------------------------------------------------------------------
# EXECUTABLE TARGET
# ----------------------------------------------------------------------------------------------------------------------

qt_add_executable(${PROJECT_TARGET_NAME}
    MANUAL_FINALIZATION
    WIN32
    ${PROJECT_HEADERS}
    ${PROJECT_INLINE}
    ${PROJECT_SOURCES}
    ${PROJECT_FORMS}
    ${PROJECT_RESOURCES}
    ${PROJECT_DOCS}
    ${PROJECT_SCRIPTS}
    ${PROJECT_WINDOWS_RESOURCES}
    ${PROJECT_LICENSE_FILE}
)

target_include_directories(${PROJECT_TARGET_NAME}
    PRIVATE
        ${PROJECT_INCLUDES_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
)

target_compile_definitions(${PROJECT_TARGET_NAME}
    PRIVATE
        PROJECT_NAME="${PROJECT_TARGET_NAME}"
        PROJECT_DISPLAY_NAME="${PROJECT_DISPLAY_NAME}"
        PROJECT_VERSION="${PROJECT_VERSION_VALUE}"
)

if(WIN32)
    target_compile_definitions(${PROJECT_TARGET_NAME}
        PRIVATE
            NOMINMAX
            WIN32_LEAN_AND_MEAN
    )
endif()

target_link_libraries(${PROJECT_TARGET_NAME}
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
)

# ----------------------------------------------------------------------------------------------------------------------
# WINDOWS GRAPHICS CAPTURE / WINDOWS SDK
# ----------------------------------------------------------------------------------------------------------------------

if(WIN32)
    target_link_libraries(${PROJECT_TARGET_NAME}
        PRIVATE
            windowsapp
            d3d11
            dxgi
            user32
            dwmapi
            gdi32
    )

    target_compile_definitions(${PROJECT_TARGET_NAME}
        PRIVATE
            SBPSHOTS_HAS_WGC=1
    )
endif()

# ----------------------------------------------------------------------------------------------------------------------
# MSVC CONFIGURATION
# ----------------------------------------------------------------------------------------------------------------------

if(MSVC)
    target_compile_options(${PROJECT_TARGET_NAME}
        PRIVATE
            /W4
            /permissive-
            /Zc:__cplusplus
            /utf-8
    )

    target_compile_definitions(${PROJECT_TARGET_NAME}
        PRIVATE
            _CRT_SECURE_NO_WARNINGS
    )
endif()

# ----------------------------------------------------------------------------------------------------------------------
# SOURCE GROUPS FOR VISUAL STUDIO / QT CREATOR
# ----------------------------------------------------------------------------------------------------------------------

source_group(
    TREE ${PROJECT_INCLUDES_DIR}
    PREFIX "Header Files"
    FILES ${PROJECT_HEADERS} ${PROJECT_INLINE}
)

source_group(
    TREE ${PROJECT_SOURCES_DIR}
    PREFIX "Source Files"
    FILES ${PROJECT_SOURCES}
)

source_group(
    TREE ${PROJECT_FORMS_DIR}
    PREFIX "Forms"
    FILES ${PROJECT_FORMS}
)

source_group(
    TREE ${PROJECT_RESOURCES_DIR}
    PREFIX "Resources"
    FILES ${PROJECT_RESOURCES}
)

# Windows resources include the generated VERSIONINFO .rc in the build tree, which is not under the
# resources directory, so group it with a flat (non-TREE) source_group.
source_group(
    "Resources"
    FILES ${PROJECT_WINDOWS_RESOURCES}
)

source_group(
    TREE ${PROJECT_DOCS_DIR}
    PREFIX "Documentation"
    FILES ${PROJECT_DOCS}
)

source_group(
    TREE ${PROJECT_SCRIPTS_DIR}
    PREFIX "Scripts"
    FILES ${PROJECT_SCRIPTS}
)

source_group(
    TREE ${CMAKE_CURRENT_SOURCE_DIR}/..
    PREFIX "Licenses"
    FILES ${PROJECT_LICENSE_FILE}
)

# ----------------------------------------------------------------------------------------------------------------------
# INSTALL / DEPLOY
# ----------------------------------------------------------------------------------------------------------------------

# This assumes this CMakeLists.txt is located at:
#
#   <repo>/SBPShots/CMakeLists.txt
#
# and the deployments should go to:
#
#   <repo>/deploys
#
# With your current paths, this should produce:
#
#   T:/workspace_qt/SBPShots/deploys/...
get_filename_component(SBPSHOTS_REPOSITORY_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    ABSOLUTE
)

set(SBPSHOTS_DEPLOYS_ROOT
    "${SBPSHOTS_REPOSITORY_ROOT}/deploys"
    CACHE PATH "Root directory for SBP Shots deployment outputs"
)

# ----------------------------------------------------------------------------------------------------------------------
# DEPLOY PLATFORM
# ----------------------------------------------------------------------------------------------------------------------

if(WIN32)
    set(SBPSHOTS_DEPLOY_PLATFORM "windows")
elseif(APPLE)
    set(SBPSHOTS_DEPLOY_PLATFORM "macos")
elseif(UNIX)
    set(SBPSHOTS_DEPLOY_PLATFORM "linux")
else()
    string(TOLOWER "${CMAKE_SYSTEM_NAME}" SBPSHOTS_DEPLOY_PLATFORM)
endif()

# ----------------------------------------------------------------------------------------------------------------------
# DEPLOY ARCHITECTURE
# ----------------------------------------------------------------------------------------------------------------------

string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" SBPSHOTS_SYSTEM_PROCESSOR_LOWER)

if(SBPSHOTS_SYSTEM_PROCESSOR_LOWER MATCHES "^(amd64|x86_64)$")
    set(SBPSHOTS_DEPLOY_ARCH "x86_64")
elseif(SBPSHOTS_SYSTEM_PROCESSOR_LOWER MATCHES "^(i386|i686|x86)$")
    set(SBPSHOTS_DEPLOY_ARCH "x86")
elseif(SBPSHOTS_SYSTEM_PROCESSOR_LOWER MATCHES "^(arm64|aarch64)$")
    set(SBPSHOTS_DEPLOY_ARCH "arm64")
elseif(SBPSHOTS_SYSTEM_PROCESSOR_LOWER MATCHES "^arm")
    set(SBPSHOTS_DEPLOY_ARCH "arm")
else()
    set(SBPSHOTS_DEPLOY_ARCH "${SBPSHOTS_SYSTEM_PROCESSOR_LOWER}")
endif()

# ----------------------------------------------------------------------------------------------------------------------
# DEPLOY COMPILER
# ----------------------------------------------------------------------------------------------------------------------

if(MSVC)
    set(SBPSHOTS_DEPLOY_COMPILER "msvc")
elseif(MINGW)
    set(SBPSHOTS_DEPLOY_COMPILER "mingw")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(SBPSHOTS_DEPLOY_COMPILER "gcc")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(SBPSHOTS_DEPLOY_COMPILER "clang")
else()
    string(TOLOWER "${CMAKE_CXX_COMPILER_ID}" SBPSHOTS_DEPLOY_COMPILER)
endif()

# Compiler version.
#
# Examples:
#   MSVC  19.44.35207 -> 19.44
#   GCC   14.2.0      -> 14.2
#   Clang 18.1.8      -> 18.1
if(CMAKE_CXX_COMPILER_VERSION)
    string(REGEX MATCH "^[0-9]+\\.[0-9]+" SBPSHOTS_DEPLOY_COMPILER_VERSION
        "${CMAKE_CXX_COMPILER_VERSION}"
    )

    if(NOT SBPSHOTS_DEPLOY_COMPILER_VERSION)
        set(SBPSHOTS_DEPLOY_COMPILER_VERSION "${CMAKE_CXX_COMPILER_VERSION}")
    endif()
else()
    set(SBPSHOTS_DEPLOY_COMPILER_VERSION "unknown")
endif()

set(SBPSHOTS_DEPLOY_TRIPLET
    "${SBPSHOTS_DEPLOY_ARCH}-${SBPSHOTS_DEPLOY_PLATFORM}-${SBPSHOTS_DEPLOY_COMPILER}-${SBPSHOTS_DEPLOY_COMPILER_VERSION}"
    CACHE STRING "SBP Shots deployment triplet"
)

# ----------------------------------------------------------------------------------------------------------------------
# DEPLOY CONFIGURATION
# ----------------------------------------------------------------------------------------------------------------------

# Qt Creator usually creates one build tree per configuration:
#
#   buildtrees/msvc2022-debug
#   buildtrees/msvc2022-release
#
# In that case, CMAKE_BUILD_TYPE should be Debug or Release.
if(CMAKE_BUILD_TYPE)
    string(TOLOWER "${CMAKE_BUILD_TYPE}" SBPSHOTS_DEPLOY_CONFIG)
else()
    # Fallback for multi-config generators.
    # If using Visual Studio or Ninja Multi-Config directly, override this manually
    # from Qt Creator/CMake cache if needed.
    set(SBPSHOTS_DEPLOY_CONFIG "release" CACHE STRING "SBP Shots deploy configuration")
endif()

# Final install prefix:
#
#   <repo>/deploys/<triplet>/<version>/<debug|release>
#
set(SBPSHOTS_INSTALL_PREFIX
    "${SBPSHOTS_DEPLOYS_ROOT}/${SBPSHOTS_DEPLOY_TRIPLET}/${PROJECT_VERSION_VALUE}/${SBPSHOTS_DEPLOY_CONFIG}"
    CACHE PATH "SBP Shots install prefix"
)


if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX
        "${SBPSHOTS_INSTALL_PREFIX}"
        CACHE PATH "Install prefix" FORCE
    )
endif()

message(STATUS "SBP Shots deploy root:       ${SBPSHOTS_DEPLOYS_ROOT}")
message(STATUS "SBP Shots deploy triplet:    ${SBPSHOTS_DEPLOY_TRIPLET}")
message(STATUS "SBP Shots deploy config:     ${SBPSHOTS_DEPLOY_CONFIG}")
message(STATUS "SBP Shots install prefix:    ${CMAKE_INSTALL_PREFIX}")
message(STATUS "SBP Shots runtime directory: ${CMAKE_INSTALL_BINDIR}")

# ----------------------------------------------------------------------------------------------------------------------
# INSTALL RULES
# ----------------------------------------------------------------------------------------------------------------------

install(TARGETS ${PROJECT_TARGET_NAME}
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_app_script(
    TARGET ${PROJECT_TARGET_NAME}
    OUTPUT_SCRIPT SBPSHOTS_DEPLOY_SCRIPT
    NO_UNSUPPORTED_PLATFORM_ERROR
    NO_TRANSLATIONS
)

install(SCRIPT ${SBPSHOTS_DEPLOY_SCRIPT})

# ----------------------------------------------------------------------------------------------------------------------
# QT FINALIZATION
# ----------------------------------------------------------------------------------------------------------------------

qt_finalize_executable(${PROJECT_TARGET_NAME})

