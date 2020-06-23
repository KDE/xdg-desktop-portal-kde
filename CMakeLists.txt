cmake_minimum_required(VERSION 3.0)

project(xdg-desktop-portal-kde)

set(PROJECT_VERSION "5.19.2")
set(PROJECT_VERSION_MAJOR 5)

set(QT_MIN_VERSION "5.14.0")
set(KF5_MIN_VERSION "5.66.0")

################# set KDE specific information #################

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(KDEClangFormat)

include(FeatureSummary)

option(ENABLE_PIPEWIRE "Disable PipeWire support. PipeWire is needed for screen sharing and remote desktop" ON)
if(ENABLE_PIPEWIRE)
    set(HAVE_PIPEWIRE_SUPPORT 1)
else()
    message(STATUS "Disabling PipeWire support")
    set(HAVE_PIPEWIRE_SUPPORT 0)
endif()
add_definitions(-DHAVE_PIPEWIRE_SUPPORT=${HAVE_PIPEWIRE_SUPPORT})

if(HAVE_PIPEWIRE_SUPPORT)
    find_package(PipeWire)
    set_package_properties(PipeWire PROPERTIES
        TYPE REQUIRED
        PURPOSE "Required for screencast portal"
    )

    find_package(GBM)
    set_package_properties(GBM PROPERTIES
        TYPE REQUIRED
        PURPOSE "Required for screencast portal"
    )

    find_package(Epoxy)
    set_package_properties(Epoxy PROPERTIES DESCRIPTION "libepoxy"
        URL "https://github.com/anholt/libepoxy"
        TYPE REQUIRED
        PURPOSE "Required for screencast portal"
    )
endif()

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

if (EXISTS "${CMAKE_SOURCE_DIR}/.git")
   add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x060000)
   add_definitions(-DKF_DISABLE_DEPRECATED_BEFORE_AND_AT=0x054200)
endif()

add_subdirectory(data)
add_subdirectory(src)

# add clang-format target for all our real source files
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
