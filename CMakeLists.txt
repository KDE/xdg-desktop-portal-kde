project(xdg-desktop-portal-kde)

cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)
set(QT_MIN_VERSION "5.4.0")

set(PROJECT_VERSION "5.14.80")
set(PROJECT_VERSION_MAJOR 5)

################# set KDE specific information #################

find_package(ECM 5.41.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

include(ECMPackageConfigHelpers)
include(ECMOptionalAddSubdirectory)
include(FeatureSummary)

find_package(GLIB2)
set_package_properties(GLIB2 PROPERTIES
    TYPE OPTIONAL
    PURPOSE "Required for screencast portal"
)

find_package(SPA)
set_package_properties(SPA PROPERTIES
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

if (${PipeWire_FOUND} AND ${SPA_FOUND} AND ${GLIB2_FOUND} AND ${GBM_FOUND} AND ${Epoxy_FOUND})
    set (SCREENCAST_ENABLED true)
else()
    set (SCREENCAST_ENABLED false)
endif()
add_definitions(-DSCREENCAST_ENABLED=${SCREENCAST_ENABLED})

add_feature_info ("Screencast portal" ${SCREENCAST_ENABLED} "Support for screen sharing")

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
    WidgetsAddons
)

add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0)
add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)
remove_definitions(-DQT_NO_CAST_FROM_ASCII -DQT_STRICT_ITERATORS -DQT_NO_CAST_FROM_BYTEARRAY)

add_subdirectory(data)
add_subdirectory(src)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
