cmake_minimum_required(VERSION 4.1.2)
project( IceBroLite )

set(CMAKE_CXX_STANDARD 20)

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

include_directories("src/imgui")

add_executable( IceBroLite
        src/6510.cpp
        src/Breakpoints.cpp
        src/C64Colors.cpp
        src/CodeColoring.cpp
        src/Commands.cpp
        src/Config.cpp
        src/Expressions.cpp
        src/FileDialog.cpp
        src/Files.cpp
        src/IceBroLite.cpp
        src/IceBroLiteC.c
        src/Icons.cpp
        src/Image.cpp
        src/ImGui_Helper.cpp
        src/Mnemonics.cpp
        src/Platform.cpp
        src/SaveState.cpp
        src/SokolImpl.cpp
        src/SourceDebug.cpp
        src/StartVice.cpp
        src/struse.cpp
        src/Sym.cpp
        src/Traces.cpp
        src/ViceInterface.cpp
        src/ViceMonitorInterface.cpp
        src/views/BreakpointView.cpp
        src/views/CodeView.cpp
        src/views/ConsoleView.cpp
        src/views/FilesView.cpp
        src/views/GfxView.cpp
        src/views/MemView.cpp
        src/views/PreView.cpp
        src/views/RegView.cpp
        src/views/ScreenView.cpp
        src/views/SectionView.cpp
        src/views/SymbolView.cpp
        src/views/ToolBar.cpp
        src/views/TraceView.cpp
        src/views/Views.cpp
        src/views/WatchView.cpp
        src/imgui/imgui.cpp
        src/imgui/imgui_draw.cpp
        src/imgui/imgui_tables.cpp
        src/imgui/imgui_widgets.cpp
        src/data/C64_Pro_Mono-STYLE.ttf.cpp
        src/struse/xml.cpp
)

if (CMAKE_SYSTEM_NAME STREQUAL Windows)
    target_link_libraries( IceBroLite ws2_32.lib ${LIBRARY_INCLUDES} )
    set_target_properties( IceBroLite PROPERTIES WIN32_EXECUTABLE TRUE )
elseif (CMAKE_SYSTEM_NAME STREQUAL Linux)
    find_package(OpenGL REQUIRED)
    find_package(X11 REQUIRED)
#    find_package(Xcursor REQUIRED)
    target_link_libraries( IceBroLite OpenGL::GL X11::X11 X11::Xcursor X11::Xi ${LIBRARY_INCLUDES} )
#    target_link_libraries( IceBroLite OpenGL::GL XLib ${LIBRARY_INCLUDES} )
endif()
