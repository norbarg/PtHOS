@echo off
cd /d %~dp0
"%MSMPI_BIN%mpiexec.exe" -n 8 main.exe
pause