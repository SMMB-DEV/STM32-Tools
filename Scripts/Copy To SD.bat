:: This script is run in [project-dir]\MDK-ARM\

@echo off

set file=%~1
set target_drive=%~2

echo Copying %file% to %target_drive%...
copy "%file%" "%target_drive%:\" /B /Y /V
