add_definitions(-DTRANSLATION_DOMAIN="xdg-desktop-portal-kde")

set(xdg_desktop_portal_kde_SRCS
    xdg-desktop-portal-kde.cpp
    appchooser.cpp
    appchooserdialog.cpp
    desktopportal.cpp
    filechooser.cpp

    # DBus related stuff
    dbus/AppChooserAdaptor.cpp
    dbus/AppChooserInterface.cpp
    dbus/FileChooserAdaptor.cpp
    dbus/FileChooserInterface.cpp
)

ki18n_wrap_ui(xdg_desktop_portal_kde_SRCS
    appchooserdialog.ui
)

add_executable(xdg-desktop-portal-kde ${xdg_desktop_portal_kde_SRCS})

target_link_libraries(xdg-desktop-portal-kde
    Qt5::Core
    Qt5::DBus
    Qt5::Widgets
    KF5::I18n
)

install(TARGETS xdg-desktop-portal-kde DESTINATION ${KDE_INSTALL_LIBEXECDIR})
