@echo off
if not exist bin mkdir bin
cl /O2 /Ibin\SDL3-x64\include src\jumbo.c bin\SDL3-x64\lib\SDL3.lib bin\SDL3-x64\lib\SDL3_image.lib /Feneske.exe /link /SUBSYSTEM:WINDOWS
copy bin\SDL3-x64\bin\* bin
move neske.exe bin
del *.obj