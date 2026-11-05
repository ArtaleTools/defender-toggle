@echo off
title Defender offline
echo Applying Defender offline change. Windows will restart automatically.
wpeinit
set "ROOT="
for /l %%N in (1,1,30) do (
  if not defined ROOT (
    for %%D in (C D E F G H I J K L M N O P Q R S T U V W Y Z) do (
      if not defined ROOT if exist "%%D:@@RELDIR@@\action.txt" set "ROOT=%%D:@@RELDIR@@"
    )
    if not defined ROOT ping -n 3 127.0.0.1 >nul
  )
)
if not defined ROOT (
  echo [ERROR] DefenderOffline folder not found. Nothing was changed. Restarting in 30 seconds.
  ping -n 31 127.0.0.1 >nul
  goto done
)
set "WINDRIVE="
for %%D in (C D E F G H I J K L M N O P Q R S T U V W Y Z) do (
  if not defined WINDRIVE if exist "%%D:\Windows\DefenderToggle.target" set "WINDRIVE=%%D:"
)
if not defined WINDRIVE (
  echo [ERROR] Target Windows not found. Nothing was changed.>"%ROOT%\last_result.log"
  del /f /q "%ROOT%\action.txt"
  goto done
)
del /f /q "%WINDRIVE%\Windows\DefenderToggle.target"
set "ACTION="
set /p ACTION=<"%ROOT%\action.txt"
del /f /q "%ROOT%\action.txt"
if /i "%ACTION%"=="disable" goto run
if /i "%ACTION%"=="restore" goto run
echo unknown action: %ACTION%>"%ROOT%\last_result.log"
goto done
:run
call "%ROOT%\%ACTION%_defender_offline.cmd" <nul >"%ROOT%\last_result.log" 2>&1
type "%ROOT%\last_result.log"
ping -n 6 127.0.0.1 >nul
:done
wpeutil reboot
exit /b 0
