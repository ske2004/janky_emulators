@echo off

rem set ROM_NAME="P:\ROMS\Bonk's Adventure (USA).pce"
rem set ROM_NAME="P:\ROMS\Bomberman (Japan).pce"
rem set ROM_NAME="P:\ROMS\Bikkuriman World (Japan).pce"
set ROM_NAME="P:\ROMS\Magical Chase (Japan).pce"
rem set ROM_NAME="P:\ROMS\Shanghai (Japan).pce"
rem set ROM_NAME="P:\ROMS\Fantasy Zone (USA).pce"

if "%1" == "prof" (
  odin run . -linker:radlink -o:speed -no-bounds-check -define:ENABLE_SPALL=true -debug -- %ROM_NAME%
) else if "%1" == "fast" (
  odin run . -linker:radlink -o:speed -no-bounds-check -no-type-assert -microarch:native -- %ROM_NAME%
) else if "%1" == "debug" (
  odin run . -linker:radlink -- %ROM_NAME%
)else if "%1" == "headless" (
  odin run . -linker:radlink -o:speed -define:ENABLE_TRACING=true -define:HEADLESS=true -debug -- %ROM_NAME%
) else (
  odin run . -linker:radlink -o:speed -define:ENABLE_TRACING=true -microarch:native -debug -- %ROM_NAME%
)
