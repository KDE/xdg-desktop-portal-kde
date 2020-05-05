cmake_minimum_required(VERSION 3.0)

project(xdg-desktop-portal-kde)

set(PROJECT_VERSION "5.18.5")
set(PROJECT_VERSION_MAJOR 5)

set(QT_MIN_VERSION "5.12.0")
set(KF5_MIN_VERSION "5.66.0")

################# set KDE specific information #################

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(KDEClangFormat)

include(FeatureSummary)

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
    URL "https://github.com/anholt/libepoxy"
    TYPE OPTIONAL
    PURPOSE "Required for screencast portal"
)

if (PipeWire_FOUND AND GBM_FOUND AND Epoxy_FOUND)
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

add_subdirectory(data)
add_subdirectory(src)

# add clang-format target for all our real source files
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
