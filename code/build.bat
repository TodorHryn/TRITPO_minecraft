@echo off

pushd ..\build

cl /nologo /W4 /wd4201 /Zi /Od /MD ..\code\main.cpp /link ..\build\glfw3.lib User32.lib Kernel32.lib Gdi32.lib Shell32.lib

popd