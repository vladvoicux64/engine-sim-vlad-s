@echo off
cd /d %~dp0

taskkill /f /IM engine-sim-app.exe
start engine-sim-app.exe

exit

