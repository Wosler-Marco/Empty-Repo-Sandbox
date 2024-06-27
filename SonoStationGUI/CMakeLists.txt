cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

add_definitions(-std=c++17)

set(CXX_FLAGS "-Wall")
set(CMAKE_CXX_FLAGS "${CXX_FLAGS}")
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

project(wosler_cpp_sonostation_gui)

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
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Utils/inc)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../External/)

find_package(Threads REQUIRED)
find_package(Qt6 COMPONENTS Core Widgets REQUIRED)
find_package(VTK COMPONENTS
  CommonColor
  CommonCore
  IOGeometry
  GUISupportQt
REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(jsoncpp REQUIRED)
pkg_check_modules(GLIB2 REQUIRED glib-2.0)
pkg_check_modules(GST_APP REQUIRED gstreamer-app-1.0)
pkg_check_modules(GOBJ2 REQUIRED gobject-2.0)
if (NOT VTK_FOUND)
  message(FATAL_ERROR "InteractWithImage: Unable to find the VTK build folder.")
endif()

find_package(OpenCV REQUIRED)
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
include_directories(${OpenCV_INCLUDE_DIRS})

file( GLOB QT_SOURCE_FILES ./inc/*.h* )
qt6_wrap_cpp(QT_SRC ${QT_SOURCE_FILES})

file( GLOB APP_SOURCES ./tests/*.cpp )
foreach( testsourcefile ${APP_SOURCES} )
  get_filename_component(testname ${testsourcefile} NAME_WE)

  add_executable(${testname} ${testsourcefile} ${QT_SRC})

  # Add DEBUG definition for Debug builds
  # target_compile_definitions(${testname} PRIVATE $<$<CONFIG:Debug>:DEBUG>)

  target_link_libraries( ${testname} Threads::Threads ${LIBUSB_LIB} Qt6::Core Qt6::Widgets ${VTK_LIBRARIES} ${OpenCV_LIBS} ${GST_SAMPLE_LIBRARIES})

  if(UNIX)
    target_link_libraries(${testname})
  elseif(WIN32)
    target_link_libraries(${testname} winMM ws2_32)
  else()
    message(FATAL_ERROR "Unkown OS!")
  endif()
endforeach( testsourcefile ${APP_SOURCES} )

