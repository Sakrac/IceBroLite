# Linux build instructions for IceBro Lite

IceBro Lite builds with CMake and Ninja on Linux.

## Prerequisites

Install the toolchain and graphics dependencies:

```bash
sudo apt update
sudo apt install -y cmake ninja-build clang llvm \
  mesa-common-dev libglu1-mesa-dev freeglut3-dev \
  libwayland-dev libxkbcommon-dev libxrandr-dev \
  libxinerama-dev libxcursor-dev libxi-dev
```

On Fedora use:

```bash
sudo dnf install cmake ninja-build clang llvm \
  mesa-libGL-devel wayland-devel libxkbcommon-devel \
  libXcursor-devel libXi-devel libXinerama-devel libXrandr-devel
```

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
