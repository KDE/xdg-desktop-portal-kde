ecm_add_tests(
    xdgshortcuttest.cpp

    LINK_LIBRARIES Qt::Test Qt::Gui
)
target_sources(xdgshortcuttest PRIVATE ${CMAKE_SOURCE_DIR}/src/xdgshortcut.cpp ${CMAKE_SOURCE_DIR}/src/xdgshortcut.h)
if (QT_MAJOR_VERSION EQUAL "6")
    target_link_libraries(xdgshortcuttest Qt::GuiPrivate)
elseif (QT_MAJOR_VERSION EQUAL "5")
    target_link_libraries(xdgshortcuttest Qt::XkbCommonSupportPrivate PkgConfig::XKB)
endif()
