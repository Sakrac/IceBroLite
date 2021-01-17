#
# Cross Platform Makefile
# Compatible with MSYS2/MINGW, Ubuntu 14.04.1 and Mac OS X
#
# You will need GLFW (http://www.glfw.org):
# Linux:
#   apt-get install libglfw-dev
# Mac OS X:
#   brew install glfw
# MSYS2:
#   pacman -S --noconfirm --needed mingw-w64-x86_64-toolchain mingw-w64-x86_64-glfw
#

CXX = g++
#CXX = clang++

EXE = ../IceBroLite.exe
SOURCES = 6510.cpp Breakpoints.cpp C64Colors.cpp Config.cpp Expressions.cpp FileDialog.cpp Files.cpp
SOURCES += IceBroLite.cpp Icons.cpp Image.cpp ImGui_Helper.cpp Mnemonics.cpp
SOURCES += Platform.cpp SourceDebug.cpp struse.cpp Sym.cpp ViceInterface.cpp ViceMonitorInterface.cpp
SOURCES += imgui/backends/imgui_impl_glfw.cpp imgui/backends/imgui_impl_opengl2.cpp
SOURCES += imgui/imgui.cpp imgui/imgui_draw.cpp imgui/imgui_tables.cpp
SOURCES += imgui/imgui_widgets.cpp
SOURCES += struse/xml.cpp
SOURCES += views/BreakpointView.cpp views/CodeView.cpp views/ConsoleView.cpp views/FilesView.cpp
SOURCES += views/ScreenView.cpp views/MemView.cpp views/RegView.cpp
SOURCES += views/SymbolView.cpp views/WatchView.cpp views/ToolBar.cpp views/Views.cpp
SOURCES += data/C64_Pro_Mono-STYLE.ttf.cpp
OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))
UNAME_S := $(shell uname -s)

CXXFLAGS = -I./imgui -I./imgui/backends
CXXFLAGS += -g -O2 -Wall -Wformat
LIBS =

##---------------------------------------------------------------------
## BUILD FLAGS PER PLATFORM
##---------------------------------------------------------------------

ifeq ($(UNAME_S), Linux) #LINUX
	ECHO_MESSAGE = "Linux"
	LIBS += -lGL `pkg-config --static --libs glfw3` -pthread

	CXXFLAGS += `pkg-config --cflags glfw3`
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(UNAME_S), Darwin) #APPLE
	ECHO_MESSAGE = "Mac OS X"
	LIBS += -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
	LIBS += -L/usr/local/lib -L/opt/local/lib
	#LIBS += -lglfw3
	LIBS += -lglfw

	CXXFLAGS += -I/usr/local/include -I/opt/local/include
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(findstring MINGW,$(UNAME_S)),MINGW)
	ECHO_MESSAGE = "MinGW"
	LIBS += -lglfw3 -lgdi32 -lopengl32 -limm32

	CXXFLAGS += `pkg-config --cflags glfw3`
	CFLAGS = $(CXXFLAGS)
endif

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:data/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:views/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:imgui/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:imgui/backends/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:struse/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

clean:
	rm -f $(EXE) $(OBJS)

