cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

add_definitions(-std=c++17)

set(CXX_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS "${CXX_FLAGS} -O1")
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

project(wosler_cpp_utils)

# Check if the ENABLE_DEBUG option is provided
option(ENABLE_DEBUG "Enable debug mode" OFF)

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
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../External/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../External/websocketpp-master)

find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED COMPONENTS thread)

file( GLOB APP_SOURCES ./tests/*.cpp )
foreach( testsourcefile ${APP_SOURCES} )
  get_filename_component(testname ${testsourcefile} NAME_WE)
  add_executable(${testname} ${testsourcefile})

  target_link_libraries( ${testname} Threads::Threads OpenSSL::SSL Boost::thread)

  if(UNIX)
    target_link_libraries(${testname})
  elseif(WIN32)
    target_link_libraries(${testname} winMM ws2_32 mswsock)
  else()
    message(FATAL_ERROR "Unkown OS!")
  endif()
endforeach( testsourcefile ${APP_SOURCES} )