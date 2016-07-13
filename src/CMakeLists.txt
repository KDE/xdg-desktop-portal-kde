add_definitions(-DTRANSLATION_DOMAIN="xdg-desktop-portal-kde")

set(xdg_desktop_portal_kde_SRCS
    xdg-desktop-portal-kde.cpp
    filechooser.cpp

    # DBus related stuff
    dbus/FileChooserAdaptor.cpp
    dbus/FileChooserInterface.cpp
)

add_executable(xdg-desktop-portal-kde ${xdg_desktop_portal_kde_SRCS})

target_link_libraries(xdg-desktop-portal-kde
    Qt5::Core
    Qt5::DBus
    Qt5::Widgets
)

install(TARGETS xdg-desktop-portal-kde DESTINATION ${KDE_INSTALL_LIBEXECDIR})
