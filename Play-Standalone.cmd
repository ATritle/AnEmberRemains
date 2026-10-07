@echo off
setlocal
cd /d "%~dp0"
if not exist "Builds\HushedCloisters\Windows\AnEmberRemains.exe" (
  echo No standalone package found. Run Tools\package_cloisters.ps1 with UE 5.8 installed first.
  pause
  exit /b 1
)
start "An Ember Remains" "Builds\HushedCloisters\Windows\AnEmberRemains.exe" -windowed -ResX=1280 -ResY=800
