find_package(Qt6 REQUIRED COMPONENTS Gui)

add_library(ag_io STATIC
    AgdDocumentIO.h
    AgdDocumentIO.cpp
    PngExporter.h
    PngExporter.cpp
)

target_include_directories(ag_io PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ag_io PUBLIC ag_engine Qt6::Gui)
