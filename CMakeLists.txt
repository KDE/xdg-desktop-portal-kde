project(xdg-desktop-portal-kde)

cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)
set(QT_MIN_VERSION "5.4.0")

set(PROJECT_VERSION "5.12.80")
set(PROJECT_VERSION_MAJOR 5)

################# set KDE specific information #################

find_package(ECM 1.3.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

include(ECMPackageConfigHelpers)
include(ECMOptionalAddSubdirectory)
include(FeatureSummary)

include(FindPkgConfig)

# remove?
pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.1)
pkg_check_modules(SPA REQUIRED libspa-0.1)
pkg_check_modules(GLIB REQUIRED glib-2.0)

find_package(gbm REQUIRED)
set_package_properties(gbm PROPERTIES DESCRIPTION "GBM - Generic Buffer Management"
    TYPE REQUIRED
    PURPOSE "Required for egl ouput of drm backend."
)

find_package(epoxy REQUIRED)
include_directories(${epoxy_INCLUDE_DIRS})
set_package_properties(epoxy PROPERTIES DESCRIPTION "libepoxy"
    URL "http://github.com/anholt/libepoxy"
    TYPE REQUIRED
    PURPOSE "OpenGL dispatch library for GBM backend"
)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Core
    DBus
    PrintSupport
    Widgets
)

find_package(KF5 REQUIRED
    CoreAddons
    I18n
    Notifications
    Wayland
)

add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0)
add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)
remove_definitions(-DQT_NO_CAST_FROM_ASCII -DQT_STRICT_ITERATORS -DQT_NO_CAST_FROM_BYTEARRAY)

add_subdirectory(data)
add_subdirectory(src)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
