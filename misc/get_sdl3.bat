@echo off
curl https://github.com/mmozeiko/build-sdl3/releases/download/2025-02-02/SDL3-x64-2025-02-02.zip -O -J -L
move SDL3-x64-2025-02-02.zip bin/sdl2.zip
tar -xf bin/sdl2.zip -C bin