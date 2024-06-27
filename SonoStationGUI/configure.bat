rmdir /Q /S .\build
if "%1" == "DEBUG" (
    cmake -S . -B ./build -G "MinGW Makefiles" -D CMAKE_PREFIX_PATH=C:\Qt\6.4.2\mingw_64 -D ENABLE_DEBUG=ON
) else (
    cmake -S . -B ./build -G "MinGW Makefiles" -D CMAKE_PREFIX_PATH=C:\Qt\6.4.2\mingw_64
)