add_subdirectory(kirigami-filepicker)

add_definitions(-DTRANSLATION_DOMAIN="xdg-desktop-portal-kde")

include_directories(${Qt5PrintSupport_PRIVATE_INCLUDE_DIRS})

set(xdg_desktop_portal_kde_SRCS
    access.cpp
    accessdialog.cpp
    accessdialog.h
    access.h
    account.cpp
    account.h
    appchooser.cpp
    appchooserdialog.cpp
    appchooserdialog.h
    appchooser.h
    background.cpp
    background.h
    desktopportal.cpp
    desktopportal.h
    email.cpp
    email.h
    filechooser.cpp
    filechooser.h
    inhibit.cpp
    inhibit.h
    notification.cpp
    notification.h
    notificationinhibition.cpp
    notificationinhibition.h
    outputsmodel.cpp
    print.cpp
    print.h
    quickdialog.cpp
    remotedesktop.cpp
    remotedesktopdialog.cpp
    remotedesktopdialog.h
    remotedesktop.h
    request.cpp
    request.h
    screencast.cpp
    screencast.h
    screencasting.cpp
    screencasting.h
    screencastwidget.cpp
    screencastwidget.h
    screenchooserdialog.cpp
    screenchooserdialog.h
    screenshot.cpp
    screenshotdialog.cpp
    screenshotdialog.h
    screenshot.h
    session.cpp
    session.h
    settings.cpp
    settings.h
    userinfodialog.cpp
    userinfodialog.h
    utils.cpp
    utils.h
    waylandintegration.cpp
    waylandintegration.h
    waylandintegration_p.h
    xdg-desktop-portal-kde.cpp
    resources.qrc
)

ki18n_wrap_ui(xdg_desktop_portal_kde_SRCS
    screenshotdialog.ui
    screenchooserdialog.ui
    remotedesktopdialog.ui
)

ecm_add_qtwayland_client_protocol(xdg_desktop_portal_kde_SRCS
    PROTOCOL ${PLASMA_WAYLAND_PROTOCOLS_DIR}/screencast.xml
    BASENAME zkde-screencast-unstable-v1
)

set_source_files_properties(../data/org.freedesktop.Accounts.User.xml PROPERTIES NO_NAMESPACE TRUE)

qt_add_dbus_interface(xdg_desktop_portal_kde_SRCS ../data/org.freedesktop.Accounts.User.xml user_interface)

add_executable(xdg-desktop-portal-kde ${xdg_desktop_portal_kde_SRCS})

target_link_libraries(xdg-desktop-portal-kde
    Qt::Core
    Qt::DBus
    Qt::Concurrent
    Qt::PrintSupport
    Qt::QuickWidgets
    Qt::Widgets
    KF5::CoreAddons
    KF5::ConfigCore
    KF5::Declarative
    KF5::I18n
    KF5::KIOFileWidgets
    KF5::Notifications
    KF5::Service
    KF5::WaylandClient
    KF5::WidgetsAddons
    KF5::WindowSystem
    KirigamiFilepicker
    Wayland::Client
)

install(TARGETS xdg-desktop-portal-kde DESTINATION ${KDE_INSTALL_LIBEXECDIR})

install(FILES
    xdg-desktop-portal-kde.notifyrc
    DESTINATION ${KDE_INSTALL_KNOTIFY5RCDIR})
