@echo off
REM This file makes a junction to combine ./Canbus files to ../libraries/ files
REM assuming a structure of BlablaCode/S60CAN/Canbus
REM and arduino-1.6.5r2\libraries\Canbus
REM
REM So, the strategy is as follows:
REM Junction ..\..\arduino-1.6.5r2\libraries\Canbus to .\Canbus
mklink /J C:\Users\awaal\VolvoHacking\arduino-1.6.5-r2\libraries\Canbus C:\Users\awaal\VolvoHacking\ArduinoCode\S60CAN\Canbus

pause > nul