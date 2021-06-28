cmake_minimum_required(VERSION 3.16)

set(CMAKE_C_STANDARD 99)

set(PROJECT_VERSION "5.22.80")
set(PROJECT_VERSION_MAJOR 5)

project(xdg-desktop-portal-kde VERSION ${PROJECT_VERSION})

set(QT_MIN_VERSION "5.15.0")
set(KF5_MIN_VERSION "5.82")

################# set KDE specific information #################

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH}  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDEClangFormat)
include(KDEGitCommitHooks)

include(FeatureSummary)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Core
    Concurrent
    DBus
    PrintSupport
    QuickWidgets
    Widgets
)

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
)
find_package(Wayland 1.15 REQUIRED COMPONENTS Client)
find_package(PlasmaWaylandProtocols REQUIRED)
find_package(QtWaylandScanner REQUIRED)

add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x050c00)
add_definitions(-DKF_DISABLE_DEPRECATED_BEFORE_AND_AT=0x054200)

add_subdirectory(data)
add_subdirectory(src)

# add clang-format target for all our real source files
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
