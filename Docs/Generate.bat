@echo off

doxygen ./Config.doxygen

set ec=%errorlevel%
if %ec% equ 0 (
	timeout /t 15
) else (
	pause
)

exit /b