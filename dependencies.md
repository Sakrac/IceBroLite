# WINDOWS

# Install CMake
# Download the correct version from
# https://cmake.org/download/

# Install Ninja, put it somewhere and add the path to the path environment variable
# https://github.com/ninja-build/ninja/releases

# Install CLANG
# Download the latest release from LLVM and copy the folders (bin, etc.) to C:\Program Files\LLVM\
# https://github.com/llvm/llvm-project/releases

# FEDORA

# Fedora make sure to upgrade first
# sudo dnf upgrade --refresh

# Install CMake and Ninja
# sudo dnf install cmake ninja

# Install the clang compiler and llvm
# sudo dnf install clang
# sudo dnf install llvm

# Run this to install OpenGL development files in Fedora
# sudo dnf install mesa-libGL-devel

# Install dependencies for GLFW in Fedora
# sudo dnf install wayland-devel libxkbcommon-devel libXcursor-devel libXi-devel libXinerama-devel libXrandr-devel

# Include GLFW source in the project
cd src
git clone https://github.com/glfw/glfw.git
cd ..

# Now build_cmake.sh / build_cmake.bat should work