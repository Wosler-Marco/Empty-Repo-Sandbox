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
set(AWS_SDK_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/external/AWS/aws-cpp-sdk-all)
set(KVS_PIC_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/external/AWS/aws-kvs-pic-sdk)
set(KVS_WEBRTC_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/external/AWS/aws-kvs-webrtc-sdk)

# Add AWS SDK lib to path for find_package
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${AWS_SDK_ROOT}/lib/cmake/)
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} ${AWS_SDK_ROOT}/lib/)

# Find AWS SDK + dependencies
find_package(ZLIB REQUIRED)
find_package(AWSSDK REQUIRED COMPONENTS core kinesisvideo)

# Include AWS SDK directories
include_directories(${AWSSDK_INCLUDE_DIR})
link_directories(${AWSSDK_LIB_DIR})

# Include AWS KVS PIC and WebRTC directories
include_directories(
  ${KVS_PIC_ROOT}/include
  ${KVS_WEBRTC_ROOT}/include
)

# Specify the AWS KVS PIC and WebRTC libraries
set(KVS_PIC_LIBS
    ${KVS_PIC_ROOT}/lib/libkvsCommonLws.a
    ${KVS_PIC_ROOT}/lib/libkvspic.a
    ${KVS_PIC_ROOT}/lib/libkvspicClient.a
    ${KVS_PIC_ROOT}/lib/libkvspicState.a
    ${KVS_PIC_ROOT}/lib/libkvspicUtils.a
)

set(KVS_WEBRTC_LIBS
    ${KVS_WEBRTC_ROOT}/lib/libkvsWebrtcClient.a
    ${KVS_WEBRTC_ROOT}/lib/libkvsWebrtcSignalingClient.a
    ${KVS_WEBRTC_ROOT}/lib/libkvsWebRtcThreadpool.a
)


# Find GStreamer Modules
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED gstreamer-1.0)
pkg_check_modules(GST_SDP REQUIRED gstreamer-sdp-1.0)
pkg_check_modules(GST_APP REQUIRED gstreamer-app-1.0)
pkg_check_modules(GST_VIDEO REQUIRED gstreamer-video-1.0)

# Include and link Gstreamer module directories
include_directories(
  ${GST_INCLUDE_DIRS}
  ${GST_SDP_INCLUDE_DIRS}
  ${GST_APP_INCLUDE_DIRS}
  ${GST_VIDEO_INCLUDE_DIRS}
)
link_directories(
  ${GST_LIBRARY_DIRS}
  ${GST_SDP_LIBRARY_DIRS}
  ${GST_APP_LIBRARY_DIRS}
  ${GST_VIDEO_LIBRARY_DIRS}
)

# Find gst-plugin-scanner binary using location of libraries as base
find_program(GST_PLUGIN_SCANNER gst-plugin-scanner
    PATHS ${GST_LIBRARY_DIRS}/../libexec/gstreamer-1.0
    NO_DEFAULT_PATH)

if(NOT GST_PLUGIN_SCANNER)
    message(FATAL_ERROR "gst-plugin-scanner not found")
endif()

# Set the GST_PLUGIN_SCANNER environment variable
set(ENV{GST_PLUGIN_SCANNER} ${GST_PLUGIN_SCANNER})

# Print the GST_PLUGIN_SCANNER value for verification
message(STATUS "GST_PLUGIN_SCANNER set to $ENV{GST_PLUGIN_SCANNER}")


# Source files
file(GLOB_RECURSE SRC_FILES src/*.c*)

# Find Threads library
find_package(Threads REQUIRED)

# Find all AWS dependency libraries
find_package(OpenSSL REQUIRED)
find_library(SRTP_LIBRARIES srtp2 REQUIRED )
find_library(USRSCTP_LIBRARIES usrsctp REQUIRED)
pkg_check_modules(LIBWEBSOCKETS REQUIRED libwebsockets)

link_directories(${LIBSRTP_LIBRARY_DIRS})
link_directories(${LIBWEBSOCKETS_LIBRARY_DIRS})


# Add main.cpp executable
add_executable(sandbox ${CMAKE_CURRENT_SOURCE_DIR}/sandbox.cpp  ${SRC_FILES})

# Link libraries and include directories as needed for main
target_link_libraries(sandbox Threads::Threads)
target_link_libraries(sandbox ${GST_LIBRARIES}
                              ${GST_SDP_LIBRARIES}
                              ${GST_APP_LIBRARIES}
                              ${GST_VIDEO_LIBRARIES}
)
target_link_libraries(sandbox ZLIB::ZLIB
                              aws-cpp-sdk-kinesisvideo
                              ${KVS_PIC_LIBS}
                              ${KVS_WEBRTC_LIBS}
                              ${OPENSSL_LIBRARIES}
                              ${SRTP_LIBRARIES}
                              ${USRSCTP_LIBRARIES}
                              ${LIBWEBSOCKETS_LIBRARIES}
)

if(UNIX)
  target_link_libraries(sandbox)
elseif(WIN32)
  target_link_libraries(sandbox winMM ws2_32 iphlpapi)
else()
  message(FATAL_ERROR "Unkown OS!")
endif()

if(BUILD_TESTS)
  # Find all test source files
  file( GLOB APP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp )

  # Compile all regular files and ignore Google Test files "gtest_*"
  foreach( testsourcefile ${APP_SOURCES} )
    if(NOT testsourcefile MATCHES "gtest_.*\\.cpp")
      get_filename_component(testname ${testsourcefile} NAME_WE)
      add_executable(${testname} ${testsourcefile}  ${SRC_FILES})

      target_link_libraries( ${testname} Threads::Threads )
      target_link_libraries(${testname} ${GST_LIBRARIES}
                                        ${GST_SDP_LIBRARIES}
                                        ${GST_APP_LIBRARIES}
                                        ${GST_VIDEO_LIBRARIES}
      )
      target_link_libraries(${testname} ZLIB::ZLIB
                                        aws-cpp-sdk-kinesisvideo
                                        ${KVS_PIC_LIBS}
                                        ${KVS_WEBRTC_LIBS}
                                        ${OPENSSL_LIBRARIES}
                                        ${SRTP_LIBRARIES}
                                        ${USRSCTP_LIBRARIES}
                                        ${LIBWEBSOCKETS_LIBRARIES}
      )

      if(UNIX)
        target_link_libraries(${testname})
      elseif(WIN32)
        target_link_libraries(${testname} winMM ws2_32 iphlpapi)
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