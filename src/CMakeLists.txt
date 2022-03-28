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

if (QT_MAJOR_VERSION EQUAL "5")
    ecm_add_qtwayland_client_protocol(xdg_desktop_portal_kde_SRCS
        PROTOCOL ${PLASMA_WAYLAND_PROTOCOLS_DIR}/screencast.xml
        BASENAME zkde-screencast-unstable-v1
    )
else()
#    qt6_generate_wayland_protocol_client_sources(xdg-desktop-portal-kde FILES ${PLASMA_WAYLAND_PROTOCOLS_DIR}/screencast.xml
#        ${CMAKE_CURRENT_SOURCE_DIR}/zkde-screencast-unstable-v1.xml
#    )
endif()


set_source_files_properties(../data/org.freedesktop.Accounts.User.xml PROPERTIES NO_NAMESPACE TRUE)
qt_add_dbus_interface(xdg_desktop_portal_kde_SRCS ../data/org.freedesktop.Accounts.User.xml user_interface)

set_source_files_properties(../data/org.kde.KIOFuse.VFS.xml PROPERTIES NO_NAMESPACE TRUE)
qt_add_dbus_interface(xdg_desktop_portal_kde_SRCS ../data/org.kde.KIOFuse.VFS.xml fuse_interface)

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
    DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR})
