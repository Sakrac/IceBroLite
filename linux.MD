# Linux build instructions for IceBro Lite

IceBro Lite builds with CMake and Ninja on Linux.

## Prerequisites

Install the toolchain and the Linux windowing/Vulkan packages required by the bundled Sokol headers in [src/sokol/sokol_app.h](src/sokol/sokol_app.h).

For a Vulkan-backed Sokol build, you do not need the older OpenGL/GLUT packages. The main requirements are the X11 windowing headers plus the Vulkan SDK packages:

```bash
sudo apt update
sudo apt install -y cmake ninja-build clang llvm \
  libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
  libwayland-dev libxkbcommon-dev \
  libvulkan-dev vulkan-validationlayers vulkan-tools
```

On Fedora use:

```bash
sudo dnf install cmake ninja-build clang llvm \
  libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel \
  libXi-devel wayland-devel libxkbcommon-devel \
  vulkan-devel vulkan-validation-layers vulkan-tools
```

If you are building a non-Vulkan Sokol backend, the package list changes accordingly; the bundled Sokol headers are the best place to confirm the exact requirements for the backend you use.

## Build

```bash
git clone https://github.com/Sakrac/IceBroLite.git
cd IceBroLite
./get_dependencies.sh
./build_cmake.sh
```

The equivalent CMake invocation is:

```bash
cmake -G Ninja -S . -B build --preset linux-release
cmake --build build
```

## Run

Run the produced executable from the repository root or another folder where the debugger can save its layout file.
