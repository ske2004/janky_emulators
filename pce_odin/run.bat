@echo off
if "%1" == "prof" (
  odin run . -o:speed -no-bounds-check -define:ENABLE_SPALL=true -debug -- "P:\ROMS\Bomberman (Japan).pce"
) else if "%1" == "fast" (
  odin run . -o:speed -no-bounds-check -microarch:native -no-type-assert -- "P:\ROMS\Bomberman (Japan).pce"
) else if "%1" == "headless" (
  odin run . -define:ENABLE_TRACING=true -define:HEADLESS=true -debug -- "P:\ROMS\Bomberman (Japan).pce"
) else (
  odin run . -o:speed -define:ENABLE_TRACING=true -microarch:native -debug -- "P:\ROMS\Bomberman (Japan).pce"
)
