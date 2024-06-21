cmake_minimum_required(VERSION 3.14 FATAL_ERROR)


# Set the project name and version
project(wosler_cpp_sandbox LANGUAGES C CXX)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CXX_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS "${CXX_FLAGS} -O1")
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
#set(CMAKE_AUTOMOC ON)
if(CMAKE_VERSION VERSION_GREATER "3.24.0")
  cmake_policy(SET CMP0135 NEW)
endif()


set(KINESIS_VIDEO_WEBRTC_CLIENT_SRC "${CMAKE_CURRENT_SOURCE_DIR}/external")

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


find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

# Find all AWS dependency libraries
find_package(OpenSSL REQUIRED)
find_library(SRTP_LIBRARIES srtp2 REQUIRED )
find_library(USRSCTP_LIBRARIES usrsctp REQUIRED)
find_package(OpenCV REQUIRED)
find_package(Qt6 COMPONENTS Core Widgets REQUIRED)

find_package(VTK COMPONENTS
  CommonColor
  CommonCore
  IOGeometry
  GUISupportQt
REQUIRED)

find_package(jsoncpp REQUIRED)
pkg_check_modules(LIBWEBSOCKETS REQUIRED libwebsockets)
pkg_check_modules(GST gstreamer-1.0)

pkg_check_modules(GLIB2 REQUIRED glib-2.0)
pkg_check_modules(GST_APP REQUIRED gstreamer-app-1.0)
pkg_check_modules(GOBJ2 REQUIRED gobject-2.0)
message("gstreamer found. Will build gstreamer samples")

set(EXTERN_INCLUDE_DIRS
${EXTERN_INCLUDE_DIRS} ${GLIB2_INCLUDE_DIRS} ${GST_INCLUDE_DIRS}
${GST_APP_INCLUDE_DIRS} ${GOBJ2_INCLUDE_DIRS} )

set(GST_SAMPLE_LIBRARIES ${GLIB2_LIBRARIES} ${GST_LIBRARIES}
                   ${GST_APP_LIBRARIES} ${GOBJ2_LIBRARIES} ${Intl})
link_directories(
${GLIB2_LIBRARY_DIRS} 
${GST_LIBRARY_DIRS}
${GST_APP_LIBRARY_DIRS}
${GOBJ2_LIBRARY_DIRS})

include_directories(${EXTERN_INCLUDE_DIRS})
include_directories(${OpenCV_INCLUDE_DIRS} ${Qt6_INCLUDE_DIRS})
set(KVS_LIBS 
     ${KINESIS_VIDEO_WEBRTC_CLIENT_SRC}/lib/libkvsWebrtcClient.so
     ${KINESIS_VIDEO_WEBRTC_CLIENT_SRC}/lib/libkvsWebrtcSignalingClient.so
     ${KINESIS_VIDEO_WEBRTC_CLIENT_SRC}/lib/libkvsWebRtcThreadpool.so
     ${KINESIS_VIDEO_WEBRTC_CLIENT_SRC}/lib/libkvsCommonLws.so
     ${KINESIS_VIDEO_WEBRTC_CLIENT_SRC}/lib/libkvspic.a
     ${KINESIS_VIDEO_WEBRTC_CLIENT_SRC}/lib/libkvspicClient.a
     ${KINESIS_VIDEO_WEBRTC_CLIENT_SRC}/lib/libkvspicState.a
     ${KINESIS_VIDEO_WEBRTC_CLIENT_SRC}/lib/libkvspicUtils.a
     )
include_directories(
 ${KINESIS_VIDEO_WEBRTC_CLIENT_SRC}/include
)

link_directories(${LIBSRTP_LIBRARY_DIRS})
link_directories(${LIBWEBSOCKETS_LIBRARY_DIRS})


add_executable(
    simpleMaster
    src/Common.c
    simpleMaster.c
    src/GstAudioVideoReceiver_video.cpp
  )
 target_link_libraries(simpleMaster 
 ${EXTRA_DEPS} 
 ${GST_SAMPLE_LIBRARIES} 
 ${KVS_LIBS}                               
 ${OPENSSL_LIBRARIES}
 ${LIBWEBSOCKETS_LIBRARIES}
  ${OpenCV_LIBS})

add_executable(
    simpleVVideo
    src/Common.c
    src/GstAudioVideoReceiver_video.cpp
    simpleViewer_withvideo.cpp
  )
 target_link_libraries(simpleVVideo 
 ${EXTRA_DEPS} 
 ${GST_SAMPLE_LIBRARIES} 
 ${KVS_LIBS}                               
 ${OPENSSL_LIBRARIES}
 ${SRTP_LIBRARIES}
 ${USRSCTP_LIBRARIES}
 ${LIBWEBSOCKETS_LIBRARIES}
 ${OpenCV_LIBS})
 
 target_compile_options(simpleVVideo PRIVATE -fpermissive)
 target_compile_options(simpleMaster PRIVATE -fpermissive)

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

#get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
#foreach(dir ${dirs})
#  message(STATUS "Include directory: ${dir}")
#endforeach()
