find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(ui_tests UiTests.cpp)
target_link_libraries(ui_tests PRIVATE ag_ui ag_engine Qt6::Test)

add_test(NAME UiTests COMMAND ui_tests)
set_tests_properties(UiTests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
