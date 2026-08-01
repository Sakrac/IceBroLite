# Build dependencies

IceBro Lite now builds with CMake and the Sokol/ImGui dependency bundle in the repository. The usual workflow is:

1. Run get_dependencies.bat on Windows or get_dependencies.sh on Linux/macOS.
2. Configure and build with CMake/Ninja or the provided scripts.

## Windows

Install:
- CMake: https://cmake.org/download/
- Ninja: https://github.com/ninja-build/ninja/releases
- LLVM/Clang: https://github.com/llvm/llvm-project/releases

The CMake presets expect Clang as the compiler.

## Linux

### Debian/Ubuntu

For a Vulkan-backed Sokol build on Linux, install the windowing and Vulkan development packages rather than the older OpenGL/GLUT stack:

```bash
sudo apt update
sudo apt install -y cmake ninja-build clang llvm \
  libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
  libwayland-dev libxkbcommon-dev \
  libvulkan-dev vulkan-validationlayers vulkan-tools
```

### Fedora

```bash
sudo dnf upgrade --refresh
sudo dnf install cmake ninja-build clang llvm \
  libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel \
  libXi-devel wayland-devel libxkbcommon-devel \
  vulkan-devel vulkan-validation-layers vulkan-tools
```

The bundled Sokol headers in [src/sokol/sokol_app.h](src/sokol/sokol_app.h) list the Linux link requirements directly; for a Vulkan backend the key extra dependency is the Vulkan SDK.

## macOS

Install Xcode command line tools and Homebrew packages:

```bash
xcode-select --install
brew install cmake ninja llvm
```

Then configure with the macOS preset:

```bash
cmake -G Ninja -S . -B build --preset macos-release
cmake --build build
```

## Runtime dependency

IceBro Lite connects to VICE, so you will also need a recent VICE build with the Remote Monitor and Binary Remote Monitor enabled.
