cmake_minimum_required(VERSION 4.1.2)

set(CMAKE_CXX_STANDARD 20)
include_directories("imgui")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_BUILD_TYPE EQUAL "Debug")
    add_compile_definitions("_DEBUG")
endif()

if (CMAKE_SYSTEM_NAME STREQUAL Windows)
    add_compile_definitions("SOKOL_D3D11")
elseif (CMAKE_SYSTEM_NAME STREQUAL Linux)
    add_compile_definitions("SOKOL_GLCORE")
elseif (CMAKE_SYSTEM_NAME STREQUAL Darwin)
    add_compile_definitions("SOKOL_METAL")
endif()

# Currently needed to distinguish between wWinMain (MSVC) and WinMain (CMake)
add_compile_definitions("_CMAKE_BUILD")
# Feature flag while getting Sokol backend to build
add_compile_definitions("_SOKOL_BUILD")


# set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "/SUBSYSTEM:WINDOWS" "/ENTRY:mainCRTStartup" )

add_executable( IceBroLite
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
        IceBroLiteC.c
        Icons.cpp
        Image.cpp
        ImGui_Helper.cpp
        Mnemonics.cpp
        Platform.cpp
        SaveState.cpp
        SokolImpl.cpp
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
        imgui/imgui.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp
        data/C64_Pro_Mono-STYLE.ttf.cpp
        struse/xml.cpp
)
if (CMAKE_SYSTEM_NAME STREQUAL Windows)
    target_link_libraries( IceBroLite ws2_32.lib ${LIBRARY_INCLUDES} )
elseif (CMAKE_SYSTEM_NAME STREQUAL Linux)
    find_package(OpenGL REQUIRED)
    find_package(X11 REQUIRED)
#    find_package(Xcursor REQUIRED)
    target_link_libraries( IceBroLite OpenGL::GL X11::X11 X11::Xcursor X11::Xi ${LIBRARY_INCLUDES} )
#    target_link_libraries( IceBroLite OpenGL::GL XLib ${LIBRARY_INCLUDES} )
endif()

set_target_properties( IceBroLite PROPERTIES WIN32_EXECUTABLE TRUE )
