# Empty-Repo-Sandbox
Empty C++ Repository Sandbox

## Setup
If CMake and GCC are not already installed on your system, for Linux run the commands

    sudo apt update
    sudo apt install cmake
    sudo apt install build-essential gdb

in the terminal. For Windows, install MSYS2 via https://www.msys2.org, following the instructions up to Step 4. Open the MSYS2 terminal with the MSYS environment (run **MSYS2 MSYS**) and run the following commands:

    pacman -S mingw-w64-x86_64-cmake
    pacman -S mingw-w64-x86_64-gcc
    pacman -S mingw-w64-x86_64-make
    pacman -S mingw-w64-x86_64-gdb

As those are installing or once they are installed, add 'C:\msys64\mingw64\bin' and 'C:\msys64\usr\bin' to the path (Search for 'Environment Variables' > 'Edit the system environment variables' > Environment Variables > System Variables > find and select the variable 'Path' > Edit > New)

## How to use
Create your classes and higher level functions as needed using header and source files and place them in their appropriate folders.
Use *sandbox.cpp* to contain any of your main executable code. Any .cpp files placed in the test folder can be compiled into executables as well using the **build --test** method as described below.
To configure and build your code, go to the terminal and use the configure and build scripts (.bat for Windows and .sh for Linux/MacOS). The options for building are defined below in the **Repository Structure** section if desired.

Test that you have understood this by compiling the *sandbox.cpp* script right now and running it. Your console should have the following:

    .\configure.bat     OR      ./configure.sh
    .\build.bat         OR      ./build.sh
    .\build\bin\sandbox.exe
    >   Sandbox running...

You can also test running a test script as follows:

    .\configure.bat -t  OR      ./configure.sh -t
    .\build.bat         OR      ./build.sh
    .\build\bin\sample_test.exe
    >   Test running...
You can also use **build.__ -a** to build all the files at once.

## Repository Structure
### inc
Contains all C and C++ headers required by the code

### src
Contains all source files that implement the C/C++ headers in the _inc_ folder

### tests
Contains all unit tests and any other scripts used in testing code functionality

### sandbox_main.cpp
The main process script

### CMakeLists.txt
The file that contains the set of directives and instructions used to link and compile the SonoBot project's source files and targets. Each of the module subfolders will be treated as a subdirectory and as such will contain their own CMakeLists.txt, allowing modules to be compiled with their own defined set of directives and instructions.

### configure.sh/configure.bat
A shell/batch script to configure the code using the _cmake_ command. By default only configures the main file (sandbox_main.cpp) to be built.

| Option                | Description                                                                   |
| :-------------------- | :---------------------------------------------------------------------------- |
| -d (--debug)          | Add the compile definition DEBUG to the target code                           |
| -t (--test)           | Build all of the .cpp files contained within the tests/ folder                |
| -u (--unittest)       | Build -t as well as any unit test files in the tests/ folder (gtest_*.cpp)    |
| -r (--remove)         | Remove the build/ folder before configuring again (full configure)            |

### build.sh/build.bat
A shell/batch script to build all configures code in the build folder using the _make_ command.

| Option                | Description                                                                   |
| :-------------------- | :---------------------------------------------------------------------------- |
| -a (--all)            | Attempt to build the files concurrently with 60% of the available CPU threads |
