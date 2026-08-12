:: Create "true" relative shortcut for windows
@echo off
set "TARGET=%COMSPEC%"
set "ARGUMENTS=/C cd .\src && .\main-windows.exe"
set "LINK=%CD%\main.lnk"
set "ICON=%CD%\src\icon.ico"

powershell -NoProfile -Command ^
  "$shell=New-Object -ComObject WScript.Shell; " ^
  "$s=$shell.CreateShortcut('%LINK%'); " ^
  "$s.TargetPath='%TARGET%'; " ^
  "$s.WorkingDirectory='';" ^
  "$s.Arguments='%ARGUMENTS%'; " ^
  "$s.IconLocation='%ICON%'; " ^
  "$s.Save()"
