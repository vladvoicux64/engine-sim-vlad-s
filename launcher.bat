@echo off
cd bin
start engine-sim-app.exe
:loop
echo Press R to restart and C to close launcher and game...
choice /c RC /n
if %errorlevel%==1 goto restart
if %errorlevel%==2 goto end
:restart
taskkill /f /IM engine-sim-app.exe
start engine-sim-app.exe
cls
goto loop
:end
taskkill /f /IM engine-sim-app.exe
exit
