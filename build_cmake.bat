REM Build the Game
cmake -G "Ninja" -S src -B build --preset "x64-release"
cmake --build build
