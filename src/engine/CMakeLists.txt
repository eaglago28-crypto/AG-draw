find_package(Qt6 REQUIRED COMPONENTS Gui)

add_library(ag_engine STATIC
    Shape.h
    Shape.cpp
    RectShape.h
    RectShape.cpp
    EllipseShape.h
    EllipseShape.cpp
    PathShape.h
    PathShape.cpp
    TextShape.h
    TextShape.cpp
    Layer.h
    Layer.cpp
    Page.h
    Document.h
    Document.cpp
    Commands.h
    Commands.cpp
)

target_include_directories(ag_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ag_engine PUBLIC Qt6::Gui)
