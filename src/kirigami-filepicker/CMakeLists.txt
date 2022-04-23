set(filepicker_lib_SRCS
    api/mobilefiledialog.cpp
    api/mobilefiledialog.h
    declarative/dirmodel.cpp
    declarative/dirmodel.h
    declarative/dirmodelutils.cpp
    declarative/dirmodelutils.h
    declarative/filechooserqmlcallback.cpp
    declarative/filechooserqmlcallback.h
    declarative/fileplacesmodel.cpp
    declarative/fileplacesmodel.h
    declarative/filepicker.qrc
)

add_library(KirigamiFilepicker STATIC ${filepicker_lib_SRCS})
target_include_directories(KirigamiFilepicker PRIVATE declarative)
target_link_libraries(KirigamiFilepicker
    Qt::Quick
    Qt::Qml
    KF5::I18n
    KF5::KIOCore
    KF5::KIOFileWidgets
)
target_include_directories(KirigamiFilepicker PUBLIC api)
