@echo off
set BUILD_FLAGS=/I. /fsanitize=address /EHsc /std:c++20 /Zi
if not exist bin mkdir bin
if not exist third_party\pge\olcPixelGameEngine.pch (
  cl %BUILD_FLAGS% /c /Ycthird_party\pge\olcPixelGameEngine.h pce\PGE_PCH_TU.cpp
  copy PGE_PCH_TU.obj bin\PGE_PCH_TU.obj
)

if not exist bin\PGE_TU.pch (
  cl %BUILD_FLAGS% /c /Yuthird_party\pge\olcPixelGameEngine.h /Fpthird_party\pge\olcPixelGameEngine.pch pce\PGE_TU.cpp
  copy PGE_TU.obj bin\PGE_TU.obj
)

cl %BUILD_FLAGS% /c /Yuthird_party\pge\olcPixelGameEngine.h /Fpthird_party\pge\olcPixelGameEngine.pch pce\Main.cpp
cl %BUILD_FLAGS% bin\PGE_TU.obj bin\PGE_PCH_TU.obj Main.obj shell32.lib /Fepce.exe
move pce.exe bin
del *.obj