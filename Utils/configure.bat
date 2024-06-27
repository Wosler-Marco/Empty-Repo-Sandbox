rmdir /Q /S .\build
if "%1" == "DEBUG" (
    cmake -S . -B ./build -G "MinGW Makefiles" -D ENABLE_DEBUG=ON
) else (
    cmake -S . -B ./build -G "MinGW Makefiles"
)