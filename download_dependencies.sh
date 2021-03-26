#!/bin/bash
set -x
set -e

mkdir -p ./ThirdParty

git.exe clone --progress -v "https://github.com/EsotericSoftware/spine-runtimes.git" "./ThirdParty/spine-runtimes"
git.exe clone --progress -v "https://github.com/libsdl-org/SDL.git" "./ThirdParty/SDL"
git.exe clone --progress -v "https://github.com/nothings/stb.git" "./ThirdParty/stb"

cp draw_arrays.patch ./ThirdParty/SDL

cd ./ThirdParty/SDL
git apply --ignore-space-change --ignore-whitespace draw_arrays.patch
rm ./draw_arrays.patch

cd ../..
