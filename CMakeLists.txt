cmake_minimum_required(VERSION 3.16)

set(CMAKE_C_STANDARD 99)

set(PROJECT_VERSION "5.25.80")
set(PROJECT_VERSION_MAJOR 5)

project(xdg-desktop-portal-kde VERSION ${PROJECT_VERSION})

set(QT_MIN_VERSION "5.15.0")
set(KF5_MIN_VERSION "5.90")
set(KDE_COMPILERSETTINGS_LEVEL "5.82")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

################# set KDE specific information #################

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDEClangFormat)
include(KDEGitCommitHooks)
include(ECMSetupVersion)
include(ECMConfiguredInstall)

include(FeatureSummary)

find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Core
    Concurrent
    DBus
    PrintSupport
    QuickWidgets
    Widgets
)
if (QT_MAJOR_VERSION EQUAL "6")
    find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS WaylandClient)
endif()

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED
    CoreAddons
    Config
    I18n
    Declarative
    KIO
    Kirigami2
    Notifications
    Plasma
    Wayland
    WidgetsAddons
    WindowSystem
    IconThemes
)
find_package(Wayland 1.15 REQUIRED COMPONENTS Client)
find_package(PlasmaWaylandProtocols 1.7.0 REQUIRED)
if (QT_MAJOR_VERSION EQUAL "5")
    find_package(QtWaylandScanner REQUIRED)
endif()

find_package(KIOFuse)
set_package_properties(KIOFuse PROPERTIES
    URL https://commits.kde.org/system/kio-fuse
    TYPE RUNTIME
    PURPOSE "Automatic mounting of remote URLs")

add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x050f00)
add_definitions(-DKF_DISABLE_DEPRECATED_BEFORE_AND_AT=0x055B00)

ecm_setup_version(${PROJECT_VERSION}
    VARIABLE_PREFIX XDPK
    VERSION_HEADER ${CMAKE_BINARY_DIR}/version.h)

add_subdirectory(data)
add_subdirectory(src)

# add clang-format target for all our real source files
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
