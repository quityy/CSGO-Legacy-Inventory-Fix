@echo off
set "VCVARS="
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" (
    set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
) else (
    echo ERROR: Could not find vcvarsall.bat. Install VS Build Tools.
	pause
    exit /b 1
)

call "%VCVARS%" x86 >nul 2>&1

cl /EHsc /O2 /W3 /std:c++17 main.cpp /Fe:inventory_fix.exe /link user32.lib /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup

if %ERRORLEVEL% NEQ 0 (
    pause
)

cl /EHsc /O2 /W3 /std:c++17 patch.cpp /Fe:inventory_fix.dll /LD /link kernel32.lib

if %ERRORLEVEL% NEQ 0 (
    pause
)