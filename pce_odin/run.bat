@echo off
if "%1" == "prof" (
  odin run . -o:speed -no-bounds-check -define:ENABLE_SPALL=true -debug -- "P:\ROMS\Bomberman (Japan).pce"
) else if "%1" == "headless" (
  odin run . -define:ENABLE_TRACING=true -define:HEADLESS=true -debug -- "P:\ROMS\Bomberman (Japan).pce"
) else (
  odin run . -define:ENABLE_TRACING=true -debug -- "P:\ROMS\Bomberman (Japan).pce"
)
