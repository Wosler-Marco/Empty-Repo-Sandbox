cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

add_definitions(-std=c++17)

set(CXX_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS "${CXX_FLAGS} -O1")
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

if(CMAKE_VERSION VERSION_GREATER "3.24.0")
  cmake_policy(SET CMP0135 NEW)
endif()

project(wosler_cpp_sandbox)

# Check which options have been provided
option(ENABLE_DEBUG "Enable debug mode" OFF)
option(BUILD_TESTS "Build test files" OFF)
option(BUILD_UNIT_TESTS "Build unit test files" OFF)

# Add the DEBUG macro if ENABLE_DEBUG is set
if(ENABLE_DEBUG)
    add_compile_definitions(DEBUG)
endif()

if(UNIX)
  add_compile_definitions(_OS_UNIX)
elseif(WIN32)
  add_compile_definitions(_OS_WINDOWS)
  add_definitions(-D_OS_WINDOWS -DNOMINMAX)
else()
  message(FATAL_ERROR "Unknown OS!")
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)

# add src collection
#
#
#
#

find_package(Threads REQUIRED)

# Add main.cpp executable
add_executable(sandbox ${CMAKE_CURRENT_SOURCE_DIR}/sandbox.cpp)

# Link libraries and include directories as needed for main
target_link_libraries(sandbox Threads::Threads)

if(BUILD_TESTS)
  # Find all test source files
  file( GLOB APP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp )

  # Compile all regular files and ignore Google Test files "gtest_*"
  foreach( testsourcefile ${APP_SOURCES} )
    if(NOT testsourcefile MATCHES "gtest_.*\\.cpp")
      get_filename_component(testname ${testsourcefile} NAME_WE)
      add_executable(${testname} ${testsourcefile})

      target_link_libraries( ${testname} Threads::Threads )

      if(UNIX)
        target_link_libraries(${testname})
      elseif(WIN32)
        target_link_libraries(${testname} winMM ws2_32)
      else()
        message(FATAL_ERROR "Unkown OS!")
      endif()
endif()
  endforeach( testsourcefile ${APP_SOURCES} )
endif()

if(BUILD_TESTS OR BUILD_UNIT_TESTS)
  # Get Google Tests library
  include(FetchContent)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
  )

  # Prevent overriding the parent project's compiler/linker settings
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  enable_testing()

  file( GLOB_RECURSE GTEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/gtest_*.cpp)

  # Discover any Google Tests within the Google Test source files
  if(GTEST_SOURCES)
    add_executable(gtest_runner ${GTEST_SOURCES})
    target_link_libraries( gtest_runner GTest::gtest_main GTest::gmock )
    include(GoogleTest)
    gtest_discover_tests(gtest_runner)
  endif()
endif()