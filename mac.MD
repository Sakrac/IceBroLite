# Build instructions for macOS

IceBro Lite uses the Sokol backend on macOS and can be built with CMake and Ninja.

## Prerequisites

Install the Xcode command line tools and required Homebrew packages:

```bash
xcode-select --install
brew install cmake ninja llvm
```

## Build

```bash
git clone https://github.com/Sakrac/IceBroLite.git
cd IceBroLite
./get_dependencies.sh
cmake -G Ninja -S . -B build --preset macos-release
cmake --build build
```

## Run

The generated binary can be launched from the repository root. If you want to keep the debugger layout and state in a project-local folder, launch it from there.

