@echo off

:: Build
echo [+] Build
rmdir build /s /q
mkdir build
cd build
cmake .. --preset windows-dx9
cmake --build . --config Release
cmake --install . --config Release
