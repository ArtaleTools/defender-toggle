@echo off
rem Run inside WinRE Command Prompt. Restores 7 Defender services to their original start values.
setlocal enabledelayedexpansion
set "HIVE=%~d0\Windows\System32\config\SYSTEM"
if not exist "%HIVE%" (
  echo [ERROR] Windows registry not found: %HIVE%
  pause
  exit /b 1
)
echo Registry: %HIVE%
reg load HKLM\OFFSYS "%HIVE%"
if errorlevel 1 (
  echo [ERROR] reg load failed. Nothing was changed.
  pause
  exit /b 1
)
set /a OK=0
set /a BAD=0
call :setone WinDefend 2
call :setone WdNisSvc 3
call :setone WdNisDrv 3
call :setone WdFilter 0
call :setone WdBoot 0
call :setone MDCoreSvc 2
call :setone SecurityHealthService 3
reg unload HKLM\OFFSYS
if errorlevel 1 (
  echo [ERROR] reg unload failed. Changes may not be saved.
  set /a BAD+=1
)
reg load HKLM\OFFSW "%~d0\Windows\System32\config\SOFTWARE"
if errorlevel 1 (
  echo   FAILED cannot load SOFTWARE hive
  set /a BAD+=1
) else (
  call :setpref
  reg unload HKLM\OFFSW
  if errorlevel 1 (
    echo [ERROR] SOFTWARE unload failed.
    set /a BAD+=1
  )
)
echo.
echo RESULT: ok=!OK! failed=!BAD!
if !BAD! gtr 0 echo [ERROR] Something failed. Take a photo of this screen.
if !OK! equ 0 echo [ERROR] No service was changed. Take a photo of this screen.
if !BAD! equ 0 if !OK! gtr 0 echo SUCCESS. Close this window, then choose Continue to start Windows.
pause
exit /b 0

:setpref
set "K=HKLM\OFFSW\Microsoft\Windows Defender\Real-Time Protection"
reg query "%K%" /v DisableRealtimeMonitoring >nul 2>&1
if errorlevel 1 (
  echo   OK     realtime preference not set
  exit /b 0
)
reg add "%K%" /v DisableRealtimeMonitoring /t REG_DWORD /d 0 /f >nul 2>&1
set "GOT="
for /f "tokens=3" %%V in ('reg query "%K%" /v DisableRealtimeMonitoring 2^>nul') do set "GOT=%%V"
if /i "!GOT!"=="0x0" (
  echo   OK     realtime preference=0
  set /a OK+=1
) else (
  echo   FAILED realtime preference now=!GOT!
  set /a BAD+=1
)
exit /b 0

:setone
for %%C in (ControlSet001 ControlSet002 ControlSet003) do (
  reg query "HKLM\OFFSYS\%%C\Services\%~1" >nul 2>&1
  if not errorlevel 1 (
    reg add "HKLM\OFFSYS\%%C\Services\%~1" /v Start /t REG_DWORD /d %~2 /f >nul 2>&1
    set "GOT="
    for /f "tokens=3" %%V in ('reg query "HKLM\OFFSYS\%%C\Services\%~1" /v Start 2^>nul') do set "GOT=%%V"
    if /i "!GOT!"=="0x%~2" (
      echo   OK     %%C\%~1 Start=%~2
      set /a OK+=1
    ) else (
      echo   FAILED %%C\%~1 now=!GOT! wanted=%~2
      set /a BAD+=1
    )
  )
)
exit /b 0
