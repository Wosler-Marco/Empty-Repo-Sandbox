rmdir /Q /S .\build
cmake -DCMAKE_BUILD_TYPE=Debug -S . -B ./build -G "MinGW Makefiles"