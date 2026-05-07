@echo off
setlocal

set "DEST_DIR=%USERPROFILE%\Numerico\WeldFormGUI_bin\solvers"
set "SRC_DIR=%CD%"

if not exist "%DEST_DIR%" (
    mkdir "%DEST_DIR%"
)

call :copy_one weldform_exp.exe
call :copy_one weldform_exp_std.exe
call :copy_one weldform_imp.exe
call :copy_one weldform_imp_std.exe

echo Done.
exit /b 0

:copy_one
if exist "%SRC_DIR%\%~1" (
    copy /Y "%SRC_DIR%\%~1" "%DEST_DIR%\%~1" >nul
    echo Copied %~1 ^> %DEST_DIR%\%~1
) else (
    echo Missing %SRC_DIR%\%~1
)
exit /b 0
