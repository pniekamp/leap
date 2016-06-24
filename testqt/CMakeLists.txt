
project(testqt)

cmake_minimum_required(VERSION 2.8.12)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build Type: Debug Release RelWithDebInfo MinSizeRel" FORCE)
endif(NOT CMAKE_BUILD_TYPE)

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib CACHE INTERNAL "")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin CACHE INTERNAL "")

if(CMAKE_SYSTEM_NAME MATCHES Linux)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif(CMAKE_SYSTEM_NAME MATCHES Linux)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math")

find_package(Threads REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)

find_path(LEAP_INCLUDE leap.h PATHS ../include NO_DEFAULT_PATH)
find_library(LEAP_LIBRARIES NAMES leap PATHS ../lib)

include_directories(${LEAP_INCLUDE})

#
# cubicspline test
#

add_executable(cubicspline cubicspline.cpp)

target_link_libraries(cubicspline ${LEAP_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} Qt5::Core Qt5::Widgets)


#
# bezier test
#

add_executable(bezier bezier.cpp)

target_link_libraries(bezier ${LEAP_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} Qt5::Core Qt5::Widgets)


#
# transform test
#

add_executable(transform shape.h transform.cpp)

target_link_libraries(transform ${LEAP_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} Qt5::Core Qt5::Widgets)


#
# polygonsetop test
#

add_executable(polygonsetop shape.h polygonsetop.cpp)

target_link_libraries(polygonsetop ${LEAP_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} Qt5::Core Qt5::Widgets)


#
# polygonbrush test
#

add_executable(polygonbrush shape.h polygonbrush.cpp)

target_link_libraries(polygonbrush ${LEAP_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} Qt5::Core Qt5::Widgets)


#
# delaunay test
#

add_executable(delaunay shape.h delaunay.cpp)

target_link_libraries(delaunay ${LEAP_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} Qt5::Core Qt5::Widgets)
