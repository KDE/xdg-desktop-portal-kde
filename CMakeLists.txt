cmake_minimum_required(VERSION 3.0)

project(xdg-desktop-portal-kde)

set(PROJECT_VERSION "5.14.2")
set(PROJECT_VERSION_MAJOR 5)

set(QT_MIN_VERSION "5.11.0")
set(KF5_MIN_VERSION "5.50.0")

################# set KDE specific information #################

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

include(FeatureSummary)

find_package(GLIB2)
set_package_properties(GLIB2 PROPERTIES
    TYPE OPTIONAL
    PURPOSE "Required for screencast portal"
)

find_package(PipeWire)
set_package_properties(PipeWire PROPERTIES
    TYPE OPTIONAL
    PURPOSE "Required for screencast portal"
)

find_package(GBM)
set_package_properties(GBM PROPERTIES
    TYPE OPTIONAL
    PURPOSE "Required for screencast portal"
)

find_package(Epoxy)
set_package_properties(Epoxy PROPERTIES DESCRIPTION "libepoxy"
    URL "http://github.com/anholt/libepoxy"
    TYPE OPTIONAL
    PURPOSE "Required for screencast portal"
)

if (PipeWire_FOUND AND GLIB2_FOUND AND GBM_FOUND AND Epoxy_FOUND)
    set (SCREENCAST_ENABLED true)
else()
    set (SCREENCAST_ENABLED false)
endif()
add_definitions(-DSCREENCAST_ENABLED=${SCREENCAST_ENABLED})

add_feature_info ("Screencast portal" ${SCREENCAST_ENABLED} "Support for screen sharing")

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    Core
    Concurrent
    DBus
    PrintSupport
    Widgets
)

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED
    CoreAddons
    I18n
    Notifications
    Wayland
    WidgetsAddons
)

add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0)
add_definitions(-DQT_USE_FAST_OPERATOR_PLUS)
remove_definitions(-DQT_NO_CAST_FROM_ASCII -DQT_STRICT_ITERATORS -DQT_NO_CAST_FROM_BYTEARRAY)

add_subdirectory(data)
add_subdirectory(src)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
