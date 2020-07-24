set(filepicker_lib_SRCS
    api/mobilefiledialog.cpp
    declarative/filechooserqmlcallback.cpp
    declarative/dirmodel.cpp
    declarative/dirmodelutils.cpp
    declarative/fileplacesmodel.cpp
    declarative/filechooserqmlcallback.cpp
    declarative/dirlister.cpp
    declarative/filepicker.qrc
)

add_library(KirigamiFilepicker STATIC ${filepicker_lib_SRCS})
target_include_directories(KirigamiFilepicker PRIVATE declarative)
target_link_libraries(KirigamiFilepicker
    Qt5::Quick
    Qt5::Qml
    KF5::I18n
    KF5::KIOCore
    KF5::KIOFileWidgets
)
target_include_directories(KirigamiFilepicker PUBLIC api)
