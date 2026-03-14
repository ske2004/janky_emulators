@echo off

rem set ROM_NAME="P:\ROMS\Bonk's Adventure (USA).pce"
rem set ROM_NAME="P:\ROMS\Bomberman (Japan).pce"
rem set ROM_NAME="P:\ROMS\Bikkuriman World (Japan).pce"
set ROM_NAME="P:\ROMS\Magical Chase (Japan).pce"
rem set ROM_NAME="P:\ROMS\Shanghai (Japan).pce"
rem set ROM_NAME="P:\ROMS\Fantasy Zone (USA).pce"
rem set ROM_NAME="P:\ROMS\CPU_Test_10.pce"
rem set ROM_NAME="P:\ROMS\240pSuite.pce"
rem set ROM_NAME="P:\ROMS\Bonk 3 - Bonk's Big Adventure (USA).pce"
rem set ROM_NAME="P:\ROMS\PCE\Mega Man 2 (World) (v0.87) (Proto) (Aftermarket) (Unl).pce"
rem set ROM_NAME="P:\ROMS\PCE\Mega Man (World) (v1.01) (Aftermarket) (Unl).pce"
rem set ROM_NAME="P:\ROMS\PCE\Super Mario Bros. (World) (v0.21) (Proto) (Aftermarket) (Unl).pce"
rem set ROM_NAME="P:\ROMS\PCE\DuckTales 2 (World) (v0.31) (Proto) (Aftermarket) (Unl).pce"
rem set ROM_NAME="P:\ROMS\PCE\Contra (World) (v0.35) (Proto) (Aftermarket) (Unl).pce"
rem set ROM_NAME="P:\ROMS\PCE\Castlevania (World) (v0.44) (Proto) (Aftermarket) (Unl).pce"
rem set ROM_NAME="P:\ROMS\PCE\After Burner II (Japan) (En).pce"

if "%1" == "prof" (
  odin run . -linker:radlink -o:speed -no-bounds-check -define:ENABLE_SPALL=true -debug -- %ROM_NAME%
) else if "%1" == "debugger" (
  odin build . -debug -linker:radlink -no-bounds-check -no-type-assert
  raddbg pce_odin.exe %ROM_NAME%
) else if "%1" == "fast" (
  odin run . -linker:radlink -o:speed -no-bounds-check -no-type-assert -microarch:native -- %ROM_NAME%
) else if "%1" == "debug" (
  odin run . -linker:radlink -- %ROM_NAME%
)else if "%1" == "headless" (
  odin run . -linker:radlink -o:speed -define:ENABLE_TRACING=true -define:HEADLESS=true -debug -- %ROM_NAME%
) else (
  odin run . -linker:radlink -o:speed -define:ENABLE_TRACING=true -microarch:native -debug -- %ROM_NAME%
)
