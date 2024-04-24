@echo off

set ALL=OFF

REM Get the number of CPU cores/threads
SET "num_cores=%NUMBER_OF_PROCESSORS%"
REM Set the desired resource limit (e.g., 80% CPU usage)
SET desired_cpu_usage=60
SET j_value=1

:loop
if "%1" == "" goto end
if /I "%1" == "-a" set ALL=ON
if /I "%1" == "--all" set ALL=ON
shift
goto loop

:end

if /I "%ALL%" == "ON" (
    REM Calculate the optimal -j value based on CPU cores and resource limit
    SET /a "j_value=(num_cores * desired_cpu_usage / 100)"
)
@REM ECHO Number of CPU cores/threads: %num_cores%
@REM ECHO Calculated -j value: %j_value%

cd ./build
mingw32-make -j %j_value%