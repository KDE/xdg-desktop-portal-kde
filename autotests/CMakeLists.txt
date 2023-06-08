ecm_add_tests(
    xdgshortcuttest.cpp

    LINK_LIBRARIES Qt::Test Qt::Gui
)
target_sources(xdgshortcuttest PRIVATE ${CMAKE_SOURCE_DIR}/src/xdgshortcut.cpp ${CMAKE_SOURCE_DIR}/src/xdgshortcut.h)
target_link_libraries(xdgshortcuttest Qt::GuiPrivate)
