find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_library(ag_ui STATIC
    MainWindow.cpp
    MainWindow.h
    SplashScreen.cpp
    SplashScreen.h
    CanvasView.cpp
    CanvasView.h
    DocumentItem.cpp
    DocumentItem.h
    ToolBox.cpp
    ToolBox.h
    PropertiesBar.cpp
    PropertiesBar.h
    LayersPanel.cpp
    LayersPanel.h
    PagesPanel.cpp
    PagesPanel.h
    MacroPanel.cpp
    MacroPanel.h
    ColorPalette.cpp
    ColorPalette.h
)

target_include_directories(ag_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ag_ui PUBLIC Qt6::Widgets ag_engine ag_io)
