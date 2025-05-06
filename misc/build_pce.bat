@echo off
if not exist bin mkdir bin
cl /O2 /Ibin\SDL3-x64\include pce\Main.c bin\SDL3-x64\lib\SDL3.lib bin\SDL3-x64\lib\SDL3_image.lib /Fepce.exe
copy bin\SDL3-x64\bin\* bin
move pce.exe bin
del *.obj