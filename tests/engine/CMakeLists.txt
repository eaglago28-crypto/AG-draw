find_package(Qt6 REQUIRED COMPONENTS Test)

add_executable(engine_tests EngineTests.cpp)
target_link_libraries(engine_tests PRIVATE ag_engine ag_io Qt6::Test)

add_test(NAME EngineTests COMMAND engine_tests)
