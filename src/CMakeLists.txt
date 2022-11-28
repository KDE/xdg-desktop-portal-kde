add_subdirectory(kirigami-filepicker)

add_definitions(-DTRANSLATION_DOMAIN="xdg-desktop-portal-kde")

include_directories(${Qt5PrintSupport_PRIVATE_INCLUDE_DIRS})

add_executable(xdg-desktop-portal-kde)

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
    portalicon.cpp
    dynamiclauncher.cpp
    dynamiclauncherdialog.cpp
)

ecm_qt_declare_logging_category(xdg_desktop_portal_kde_SRCS
    IDENTIFIER "XdgDesktopPortalKdeBackground"
    CATEGORY_NAME "xdp-kde-background"
    HEADER background_debug.h)
ecm_qt_declare_logging_category(xdg_desktop_portal_kde_SRCS
    IDENTIFIER "XdgDesktopPortalKdeNotificationInhibition"
    CATEGORY_NAME "xdp-kde-notificationinhibition"
    HEADER notificationinhibition_debug.h)
ecm_qt_declare_logging_category(xdg_desktop_portal_kde_SRCS
    IDENTIFIER "XdgDesktopPortalKdeAccess"
    CATEGORY_NAME "xdp-kde-access"
    HEADER access_debug.h)
ecm_qt_declare_logging_category(xdg_desktop_portal_kde_SRCS
    IDENTIFIER "XdgDesktopPortalKdeAccount"
    CATEGORY_NAME "xdp-kde-account"
    HEADER account_debug.h)
ecm_qt_declare_logging_category(xdg_desktop_portal_kde_SRCS
    IDENTIFIER "XdgDesktopPortalKdeSettings"
    CATEGORY_NAME "xdp-kde-settings"
    HEADER settings_debug.h)
ecm_qt_declare_logging_category(xdg_desktop_portal_kde_SRCS
    IDENTIFIER "XdgDesktopPortalKdeAppChooser"
    CATEGORY_NAME "xdp-kde-app-chooser"
    HEADER appchooser_debug.h)
ecm_qt_declare_logging_category(xdg_desktop_portal_kde_SRCS
    IDENTIFIER "XdgDesktopPortalKdeDesktopPortal"
    CATEGORY_NAME "xdp-kde-desktop-portal"
    HEADER desktopportal_debug.h)
ecm_qt_declare_logging_category(xdg_desktop_portal_kde_SRCS
    IDENTIFIER "XdgDesktopPortalKdeDynamicLauncher"
    CATEGORY_NAME "xdp-kde-dynamic-launcher"
    HEADER dynamiclauncher_debug.h)
ecm_qt_declare_logging_category(xdg_desktop_portal_kde_SRCS
    IDENTIFIER "XdgDesktopPortalKdeDynamicLauncherDialog"
    CATEGORY_NAME "xdp-kde-dynamic-launcher-dialog"
    HEADER dynamiclauncherdialog_debug.h)
ecm_qt_declare_logging_category(xdg_desktop_portal_kde_SRCS
    IDENTIFIER "XdgDesktopPortalKdeEmail"
    CATEGORY_NAME "xdp-kde-email"
    HEADER email_debug.h)
ecm_qt_declare_logging_category(xdg_desktop_portal_kde_SRCS
    IDENTIFIER "XdgDesktopPortalKdeFileChooser"
    CATEGORY_NAME "xdp-kde-file-chooser"
    HEADER filechooser_debug.h)
ecm_qt_declare_logging_category(xdg_desktop_portal_kde_SRCS
    IDENTIFIER "XdgDesktopPortalKdeInhibit"
    CATEGORY_NAME "xdp-kde-inhibit"
    HEADER inhibit_debug.h)
ecm_qt_declare_logging_category(xdg_desktop_portal_kde_SRCS
    IDENTIFIER "XdgDesktopPortalKdeNotification"
    CATEGORY_NAME "xdp-kde-notification"
    HEADER notification_debug.h)
ecm_qt_declare_logging_category(xdg_desktop_portal_kde_SRCS
    IDENTIFIER "XdgDesktopPortalKdePrint"
    CATEGORY_NAME "xdp-kde-print"
    HEADER print_debug.h)
ecm_qt_declare_logging_category(xdg_desktop_portal_kde_SRCS
    IDENTIFIER "XdgDesktopPortalKdeRemoteDesktop"
    CATEGORY_NAME "xdp-kde-remotedesktop"
    HEADER remotedesktop_debug.h)
ecm_qt_declare_logging_category(xdg_desktop_portal_kde_SRCS
    IDENTIFIER "XdgDesktopPortalKdeRemoteDesktopDialog"
    CATEGORY_NAME "xdp-kde-remote-desktop-dialog"
    HEADER remotedesktopdialog_debug.h)
ecm_qt_declare_logging_category(xdg_desktop_portal_kde_SRCS
    IDENTIFIER "XdgRequestKdeRequest"
    CATEGORY_NAME "xdp-kde-request"
    HEADER request_debug.h)
ecm_qt_declare_logging_category(xdg_desktop_portal_kde_SRCS
    IDENTIFIER "XdgDesktopPortalKdeScreenCast"
    CATEGORY_NAME "xdp-kde-screencast"
    HEADER screencast_debug.h)
ecm_qt_declare_logging_category(xdg_desktop_portal_kde_SRCS
    IDENTIFIER "XdgDesktopPortalKdeScreenshot"
    CATEGORY_NAME "xdp-kde-screenshot"
    HEADER screenshot_debug.h)
ecm_qt_declare_logging_category(xdg_desktop_portal_kde_SRCS
    IDENTIFIER "XdgDesktopPortalKdeScreenshotDialog"
    CATEGORY_NAME "xdp-kde-screenshot-dialog"
    HEADER screenshotdialog_debug.h)
ecm_qt_declare_logging_category(xdg_desktop_portal_kde_SRCS
    IDENTIFIER "XdgSessionKdeSession"
    CATEGORY_NAME "xdp-kde-session"
    HEADER session_debug.h)
ecm_qt_declare_logging_category(xdg_desktop_portal_kde_SRCS
    IDENTIFIER "XdgDesktopPortalKde"
    CATEGORY_NAME "xdp-kde"
    HEADER debug.h)
ecm_qt_declare_logging_category(xdg_desktop_portal_kde_SRCS
    IDENTIFIER "XdgDesktopPortalKdeWaylandIntegration"
    CATEGORY_NAME "xdp-kde-wayland-integration"
    HEADER waylandintegration_debug.h)
ecm_qt_declare_logging_category(xdg_desktop_portal_kde_SRCS
    IDENTIFIER "XdgDesktopPortalKdeAccessDialog"
    CATEGORY_NAME "xdp-kde-access-dialog"
    HEADER accessdialog_debug.h)

if (QT_MAJOR_VERSION EQUAL "5")
    ecm_add_qtwayland_client_protocol(xdg_desktop_portal_kde_SRCS
        PROTOCOL ${PLASMA_WAYLAND_PROTOCOLS_DIR}/zkde-screencast-unstable-v1.xml
        BASENAME zkde-screencast-unstable-v1
    )
else()
    qt6_generate_wayland_protocol_client_sources(xdg-desktop-portal-kde FILES ${PLASMA_WAYLAND_PROTOCOLS_DIR}/zkde-screencast-unstable-v1.xml)
endif()


set_source_files_properties(../data/org.freedesktop.Accounts.User.xml PROPERTIES NO_NAMESPACE TRUE)
qt_add_dbus_interface(xdg_desktop_portal_kde_SRCS ../data/org.freedesktop.Accounts.User.xml user_interface)

set_source_files_properties(../data/org.freedesktop.portal.Documents.xml PROPERTIES NO_NAMESPACE TRUE)
qt_add_dbus_interface(xdg_desktop_portal_kde_SRCS ../data/org.freedesktop.portal.Documents.xml documents_interface)

set_source_files_properties(../data/org.kde.KIOFuse.VFS.xml PROPERTIES NO_NAMESPACE TRUE)
qt_add_dbus_interface(xdg_desktop_portal_kde_SRCS ../data/org.kde.KIOFuse.VFS.xml fuse_interface)

qt_add_dbus_interface(xdg_desktop_portal_kde_SRCS ../data/org.freedesktop.Application.xml fdo_application_interface)

target_sources(xdg-desktop-portal-kde PRIVATE ${xdg_desktop_portal_kde_SRCS})

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
    KF5::IconThemes
    KirigamiFilepicker
    Wayland::Client
)
if (QT_MAJOR_VERSION EQUAL "6")
    target_link_libraries(xdg-desktop-portal-kde Qt::PrintSupportPrivate)
endif()

install(TARGETS xdg-desktop-portal-kde DESTINATION ${KDE_INSTALL_LIBEXECDIR})

install(FILES
    xdg-desktop-portal-kde.notifyrc
    DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR})
