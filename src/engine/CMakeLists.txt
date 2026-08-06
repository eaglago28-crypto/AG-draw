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
    BitmapTracer.h
    BitmapTracer.cpp
    TextShape.h
    TextShape.cpp
    CompoundPathShape.h
    CompoundPathShape.cpp
    TextToCurves.h
    TextToCurves.cpp
    BrushStroke.h
    BrushStroke.cpp
    PowerClipGroup.h
    PowerClipGroup.cpp
    Layer.h
    Layer.cpp
    Page.h
    Macro.h
    Macro.cpp
    Document.h
    Document.cpp
    Commands.h
    Commands.cpp
)

target_include_directories(ag_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ag_engine PUBLIC Qt6::Gui)
