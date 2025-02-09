@echo off
if not exist bin mkdir bin
cl /fsanitize=address /Ibin\SDL3-x64\include neske.c bin\SDL3-x64\lib\SDL3.lib /Z7
copy bin\SDL3-x64\bin\* bin
move neske.exe bin
del *.obj