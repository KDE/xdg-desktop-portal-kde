add_definitions(-DTRANSLATION_DOMAIN="xdg-desktop-portal-kde")

include_directories(${Qt5PrintSupport_PRIVATE_INCLUDE_DIRS})

set(xdg_desktop_portal_kde_SRCS
    xdg-desktop-portal-kde.cpp
    access.cpp
    accessdialog.cpp
    account.cpp
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
    userinfodialog.cpp
    waylandintegration.cpp
    screencast.cpp
    screencasting.cpp
    screencastwidget.cpp
    screenchooserdialog.cpp
    remotedesktop.cpp
    remotedesktopdialog.cpp
)

ki18n_wrap_ui(xdg_desktop_portal_kde_SRCS
    accessdialog.ui
    appchooserdialog.ui
    screenshotdialog.ui
    userinfodialog.ui
    screenchooserdialog.ui
    remotedesktopdialog.ui
)

ecm_add_qtwayland_client_protocol(xdg_desktop_portal_kde_SRCS
    PROTOCOL ${PLASMA_WAYLAND_PROTOCOLS_DIR}/screencast.xml
    BASENAME zkde-screencast-unstable-v1
)

set_source_files_properties(../data/org.freedesktop.Accounts.User.xml PROPERTIES NO_NAMESPACE TRUE)

qt5_add_dbus_interface(xdg_desktop_portal_kde_SRCS ../data/org.freedesktop.Accounts.User.xml user_interface)

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

    Wayland::Client
)

install(TARGETS xdg-desktop-portal-kde DESTINATION ${KDE_INSTALL_LIBEXECDIR})

install(FILES
    qml/AppChooserDialog.qml
    qml/UserInfoDialog.qml
    DESTINATION ${KDE_INSTALL_DATADIR}/xdg-desktop-portal-kde/qml)

install(FILES
    xdg-desktop-portal-kde.notifyrc
    DESTINATION ${KNOTIFYRC_INSTALL_DIR})
