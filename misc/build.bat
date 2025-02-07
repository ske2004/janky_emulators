@echo off
if not exist bin mkdir bin
cl neske.c
move neske.pdb bin
move neske.exe bin
del *.obj