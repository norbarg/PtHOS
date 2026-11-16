@echo off

call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64

if "%~1"=="" (
    echo Usage: build.bat filename.cpp
    exit /b 1
)

cl.exe /EHsc /Zi /nologo ^
 /I "C:\Program Files (x86)\Microsoft SDKs\MPI\Include" ^
 "%~1" ^
 /Fe:"%~n1.exe" ^
 /link ^
 /LIBPATH:"C:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x64" ^
 msmpi.lib