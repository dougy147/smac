:: Create "true" relative shortcut for windows
@echo off
set "TARGET=%CD%\src\main-windows.exe"
set "LINK=%CD%\main.lnk"
::set "ICON=%CD%\src\mgico.ico"

powershell -NoProfile -Command ^
  "$shell=New-Object -ComObject WScript.Shell; " ^
  "$s=$shell.CreateShortcut('%LINK%'); " ^
  "$s.TargetPath='powershell.exe'; " ^
  "$s.WorkingDirectory='%CD%\src';" ^
  "$s.Arguments='-NoProfile -WindowStyle Hidden -ExecutionPolicy Bypass -Command & \"%TARGET%\"'; " ^
  "$s.Save()"

:: put this before Save() if ICON path provided  
  ::  "$s.IconLocation='%ICON%'; " ^
