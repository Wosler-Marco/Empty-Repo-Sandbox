@echo off

set ENABLE_DEBUG=OFF
set BUILD_TESTS=OFF
set BUILD_UNIT_TESTS=OFF
set REMOVE_BUILD=false

:loop
if "%1" == "" goto end
if /I "%1" == "-d" set ENABLE_DEBUG=ON
if /I "%1" == "--debug" set ENABLE_DEBUG=ON
if /I "%1" == "-t" set BUILD_TESTS=ON
if /I "%1" == "--test" set BUILD_TESTS=ON
if /I "%1" == "-u" set BUILD_UNIT_TESTS=ON
if /I "%1" == "--unittest" set BUILD_UNIT_TESTS=ON
if /I "%1" == "-r" set REMOVE_BUILD=true
if /I "%1" == "--remove" set REMOVE_BUILD=true
shift
goto loop

:end

:: Check if REMOVE_BUILD is true and echo the message
if "%REMOVE_BUILD%" == "true" (
    rmdir /Q /S .\build
)

cmake -S . -B ./build -G "MinGW Makefiles" -DENABLE_DEBUG=%ENABLE_DEBUG% -DBUILD_TESTS=%BUILD_TESTS% -DBUILD_UNIT_TESTS=%BUILD_UNIT_TESTS%