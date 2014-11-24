@echo off
mkdir .build
cd .build
"../cmake/bin/cmake.exe" -G "Visual Studio 12" ..
