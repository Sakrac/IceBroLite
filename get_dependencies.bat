cd src

REM Dear ImGUI
git clone --depth=1 --branch docking https://github.com/ocornut/imgui.git
cd imgui
git checkout 5098ce161db7f3919630b2b575347fc3b2ce5c62
cd ..

REM Sokol
git clone --depth=1 https://github.com/floooh/sokol.git
cd sokol
git checkout 28f9d8d44d92dab8536791a9f7d13d7e911a2b39
cd ..

cd ..