# xdg-desktop-portal-kde

A backend implementation for [xdg-desktop-portal](http://github.com/flatpak/xdg-desktop-portal)
that is using Qt/KDE.

## Building xdg-desktop-portal-gtk

### Dependencies:
 - xdg-desktop-portal (runtime)
 - flatpak (runtime)
 - Qt 5 (build)
 - KDE Frameworks (build)

### Build instructions:
```
$ cd src/dbus && ./generate-interfaces-adaptors.sh
$ cd ../..
$ mkdir build && cd build
$ cmake .. [your_options]
$ make -j5
$ make install
```
