ecm_add_tests(
    xdgshortcuttest.cpp

    LINK_LIBRARIES Qt::Test Qt::Gui
)
target_link_libraries(xdgshortcuttest Qt::GuiPrivate xdg-desktop-portal-kde-internal)

add_test(
    NAME colorschemetest
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/colorschemetest.py
)
set_tests_properties(colorschemetest PROPERTIES ENVIRONMENT "CMAKE_RUNTIME_OUTPUT_DIRECTORY=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

ecm_add_test(inputcapturebarriertest.cpp TEST_NAME inputcapturebarriertest LINK_LIBRARIES xdg-desktop-portal-kde-internal Qt::Test)

ecm_add_test(settingsportaltest.cpp TEST_NAME settingsportaltest LINK_LIBRARIES Qt::Test Qt::DBus xdg-desktop-portal-kde-internal)
