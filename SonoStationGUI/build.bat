SETLOCAL ENABLEDELAYEDEXPANSION

cd ./build
if "%1" == "ALL" (
    REM Get the number of CPU cores/threads
    SET num_cores=%NUMBER_OF_PROCESSORS%
    ECHO Number of CPU cores/threads: %num_cores%

    REM Set the desired resource limit (e.g., 80% CPU usage)
    SET desired_cpu_usage=60

    REM Calculate the optimal -j value based on CPU cores and resource limit
    SET /a "j_value=(num_cores * desired_cpu_usage / 100)"
    ECHO Calculated -j value: %j_value%

    mingw32-make -j !j_value!
) else (
    mingw32-make
)
