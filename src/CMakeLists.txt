add_definitions(-DTRANSLATION_DOMAIN="xdg-desktop-portal-kde")

include_directories(${Qt5PrintSupport_PRIVATE_INCLUDE_DIRS})

set(xdg_desktop_portal_kde_SRCS
    xdg-desktop-portal-kde.cpp
    access.cpp
    accessdialog.cpp
    appchooser.cpp
    appchooserdialog.cpp
    background.cpp
    desktopportal.cpp
    email.cpp
    filechooser.cpp
    inhibit.cpp
    notification.cpp
    print.cpp
    request.cpp
    session.cpp
    screenshot.cpp
    screenshotdialog.cpp
    settings.cpp
    utils.cpp
)

if (SCREENCAST_ENABLED)
    set (xdg_desktop_portal_kde_SRCS
         ${xdg_desktop_portal_kde_SRCS}
         screencast.cpp
         screencaststream.cpp
         screencastwidget.cpp
         screenchooserdialog.cpp
         remotedesktop.cpp
         remotedesktopdialog.cpp
         waylandintegration.cpp)

    ki18n_wrap_ui(xdg_desktop_portal_kde_SRCS
                  screenchooserdialog.ui
                  remotedesktopdialog.ui)
endif()

ki18n_wrap_ui(xdg_desktop_portal_kde_SRCS
    accessdialog.ui
    appchooserdialog.ui
    screenshotdialog.ui
)

add_executable(xdg-desktop-portal-kde ${xdg_desktop_portal_kde_SRCS})

target_link_libraries(xdg-desktop-portal-kde
    Qt5::Core
    Qt5::DBus
    Qt5::Concurrent
    Qt5::PrintSupport
    Qt5::QuickWidgets
    Qt5::Widgets
    KF5::CoreAddons
    KF5::ConfigCore
    KF5::Declarative
    KF5::I18n
    KF5::KIOFileWidgets
    KF5::Notifications
    KF5::WaylandClient
    KF5::WidgetsAddons
    KF5::WindowSystem
)

if (SCREENCAST_ENABLED)
    target_link_libraries(xdg-desktop-portal-kde
                          PipeWire::PipeWire
                          ${Epoxy_LIBRARIES}
                          GBM::GBM)
endif()

install(TARGETS xdg-desktop-portal-kde DESTINATION ${KDE_INSTALL_LIBEXECDIR})

install(FILES
    qml/AppChooserDialog.qml
    DESTINATION ${KDE_INSTALL_DATADIR}/xdg-desktop-portal-kde/qml)
