@echo off
rem Run inside WinRE Command Prompt. Sets 7 Defender services to disabled (Start=4) offline.
setlocal enabledelayedexpansion
rem WINDRIVE comes from df_run.cmd; boot images made by v1.0.0/v1.0.1 don't set it
if not defined WINDRIVE set "WINDRIVE=%~d0"
set "HIVE=%WINDRIVE%\Windows\System32\config\SYSTEM"
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
call :setone WinDefend 4
call :setone WdNisSvc 4
call :setone WdNisDrv 4
call :setone WdFilter 4
call :setone WdBoot 4
call :setone MDCoreSvc 4
call :setone SecurityHealthService 4
reg unload HKLM\OFFSYS
if errorlevel 1 (
  echo [ERROR] reg unload failed. Changes may not be saved.
  set /a BAD+=1
)
echo.
echo RESULT: ok=!OK! failed=!BAD!
if !BAD! gtr 0 echo [ERROR] Something failed. Take a photo of this screen.
if !OK! equ 0 echo [ERROR] No service was changed. Take a photo of this screen.
if !BAD! equ 0 if !OK! gtr 0 echo SUCCESS. Close this window, then choose Continue to start Windows.
pause
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
