cmake_minimum_required(VERSION 4.2)
project(src)

set(CMAKE_CXX_STANDARD 20)
include_directories("imgui" "imgui/backends" "imgui/examples/libs/glfw/include" )

if(CMAKE_BUILD_TYPE EQUAL "Debug")
    add_compile_definitions("_DEBUG")
endif()

# Currently needed to distinguish between wWinMain (MSVC) and WinMain (CMake)
add_compile_definitions("_CMAKE_BUILD")

#list( APPEND LIBRARY_INCLUDES ${CMAKE_SOURCE_DIR}/imgui/examples/libs/glfw/lib-vc2010-64/glfw3.lib )
list( APPEND LIBRARY_INCLUDES glfw )

# set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "/SUBSYSTEM:WINDOWS" "/ENTRY:mainCRTStartup" )

add_subdirectory( glfw )

add_executable( src
        6510.cpp
        Breakpoints.cpp
        C64Colors.cpp
        CodeColoring.cpp
        Commands.cpp
        Config.cpp
        Expressions.cpp
        FileDialog.cpp
        Files.cpp
        IceBroLite.cpp
        Icons.cpp
        Image.cpp
        ImGui_Helper.cpp
        Mnemonics.cpp
        Platform.cpp
        SaveState.cpp
        SourceDebug.cpp
        StartVice.cpp
        struse.cpp
        Sym.cpp
        Traces.cpp
        ViceInterface.cpp
        ViceMonitorInterface.cpp
        views/BreakpointView.cpp
        views/CodeView.cpp
        views/ConsoleView.cpp
        views/FilesView.cpp
        views/GfxView.cpp
        views/MemView.cpp
        views/PreView.cpp
        views/RegView.cpp
        views/ScreenView.cpp
        views/SectionView.cpp
        views/SymbolView.cpp
        views/ToolBar.cpp
        views/TraceView.cpp
        views/Views.cpp
        views/WatchView.cpp
        ImGUI/imgui.cpp
        ImGUI/imgui_draw.cpp
        ImGUI/imgui_tables.cpp
        ImGUI/imgui_widgets.cpp
        ImGUI/backends/imgui_impl_glfw.cpp
        ImGUI/backends/imgui_impl_opengl2.cpp
        data/C64_Pro_Mono-STYLE.ttf.cpp
        struse/xml.cpp
)

target_link_libraries( src opengl32.lib gdi32.lib ${LIBRARY_INCLUDES} ws2_32.lib )

set_target_properties( src PROPERTIES WIN32_EXECUTABLE TRUE )
