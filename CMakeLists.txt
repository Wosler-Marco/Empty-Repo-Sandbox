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

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/inc
  ${CMAKE_CURRENT_SOURCE_DIR}/external
)

# Set the path to your AWS SDK build
set(AWS_SDK_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/external/aws-cpp-sdk-all)

# Add AWS SDK lib to path for find_package
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${AWS_SDK_ROOT}/lib/cmake/)
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${AWS_SDK_ROOT}/lib/)

find_package(PkgConfig REQUIRED)
# pkg_search_module(gstreamer REQUIRED IMPORTED_TARGET gstreamer-1.0>=1.4)
# pkg_search_module(gstreamer-sdp REQUIRED IMPORTED_TARGET gstreamer-sdp-1.0>=1.4)
# pkg_search_module(gstreamer-app REQUIRED IMPORTED_TARGET gstreamer-app-1.0>=1.4)
# pkg_search_module(gstreamer-video REQUIRED IMPORTED_TARGET gstreamer-video-1.0>=1.4)
# find_package(gstreamer REQUIRED COMPONENTS sdp app video)

# Source files
file(GLOB_RECURSE SRC_FILES src/*.cpp)

# Find Threads library
find_package(Threads REQUIRED)

# Find AWS SDK + dependencies
find_package(ZLIB REQUIRED)
find_package(AWSSDK REQUIRED COMPONENTS core kinesisvideo)

# Include directories
include_directories(${AWSSDK_INCLUDE_DIR})
link_directories(${AWSSDK_LIB_DIR})

# Add main.cpp executable
add_executable(sandbox ${CMAKE_CURRENT_SOURCE_DIR}/sandbox.cpp  ${SRC_FILES})

# Link libraries and include directories as needed for main
target_link_libraries(sandbox Threads::Threads)
# target_link_libraries(sandbox PkgConfig::gstreamer
#                               PkgConfig::gstreamer-sdp
#                               PkgConfig::gstreamer-app
#                               PkgConfig::gstreamer-video
# )
target_link_libraries(sandbox ZLIB::ZLIB aws-cpp-sdk-kinesisvideo)

if(BUILD_TESTS)
  # Find all test source files
  file( GLOB APP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp )

  # Compile all regular files and ignore Google Test files "gtest_*"
  foreach( testsourcefile ${APP_SOURCES} )
    if(NOT testsourcefile MATCHES "gtest_.*\\.cpp")
      get_filename_component(testname ${testsourcefile} NAME_WE)
      add_executable(${testname} ${testsourcefile}  ${SRC_FILES})

      target_link_libraries( ${testname} Threads::Threads )
      # target_link_libraries(${testname} PkgConfig::gstreamer
      #                                   PkgConfig::gstreamer-sdp
      #                                   PkgConfig::gstreamer-app
      #                                   PkgConfig::gstreamer-video
      # )
      target_link_libraries(${testname} ZLIB::ZLIB aws-cpp-sdk-kinesisvideo)

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
    add_executable(gtest_runner ${GTEST_SOURCES} ${SRC_FILES})
    target_link_libraries( gtest_runner GTest::gtest_main GTest::gmock )
    include(GoogleTest)
    gtest_discover_tests(gtest_runner)
  endif()
endif()