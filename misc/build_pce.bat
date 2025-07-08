@echo off
if not exist bin mkdir bin
if not exist bin\PGE_TU.obj (
  cl /I. /fsanitize=address /EHsc /std:c++20 /Zi pce\PGE_TU.cpp /c
  copy PGE_TU.obj bin\PGE_TU.obj
)

cl /I. /std:c++20 /Zi /EHsc pce\Main.cpp bin\PGE_TU.obj shell32.lib /Fepce.exe /fsanitize=address 
move pce.exe bin
del *.obj