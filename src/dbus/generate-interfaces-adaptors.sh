#! /bin/sh

# TODO: do not hardcore path to xml files

DESKTOP_PORTALS=( # Access - missing Qt annotations
                 "AppChooser"
                 "FileChooser"
                 "Inhibit"
                 "Notification"
                 "Print"
                 "Request"
                 "Screenshot")

for x in ${DESKTOP_PORTALS[@]}; do
    qdbusxml2cpp -N -m -l ${x} -i "`echo "$x" | awk '{print tolower($0)}'`.h" -a "${x}Adaptor" "/usr/share/dbus-1/interfaces/org.freedesktop.impl.portal.${x}.xml"
    qdbusxml2cpp -N -m -p "${x}Interface" "/usr/share/dbus-1/interfaces/org.freedesktop.impl.portal.${x}.xml"
done
