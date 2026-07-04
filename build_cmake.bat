REM Build the Game
cmake -G "Ninja" -S . -B build --preset "x64-release"
cmake --build build
