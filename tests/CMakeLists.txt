#
# libleap tests
#

if(UNIX OR MINGW)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-parameter")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -ffast-math")
endif(UNIX OR MINGW)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-braces")
endif()

if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:__cplusplus")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4244 /wd4800 /wd4267 /wd4814")
endif(MSVC)

if(MINGW)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mthreads")
endif(MINGW)

if(WIN32)
  add_definitions(-DWIN32_LEAN_AND_MEAN -DNOMINMAX -D_CRT_SECURE_NO_WARNINGS -D_WINSOCK_DEPRECATED_NO_WARNINGS)
endif(WIN32)

include_directories(${PROJECT_SOURCE_DIR}/include)

set(SRCS ${SRCS} main.cpp)
set(SRCS ${SRCS} vector.cpp)
set(SRCS ${SRCS} matrix.cpp)
set(SRCS ${SRCS} evaluator.cpp)
set(SRCS ${SRCS} interpolator.cpp)
set(SRCS ${SRCS} siglib.cpp)
set(SRCS ${SRCS} sapstream.cpp)
set(SRCS ${SRCS} threadcontrol.cpp)
set(SRCS ${SRCS} sockets.cpp)
set(SRCS ${SRCS} regex.cpp)
set(SRCS ${SRCS} util.cpp)
set(SRCS ${SRCS} pathstring.cpp)
set(SRCS ${SRCS} http.cpp)
set(SRCS ${SRCS} geometry.cpp)
set(SRCS ${SRCS} bound.cpp)
set(SRCS ${SRCS} rtree.cpp)
set(SRCS ${SRCS} array.cpp)

add_executable(tests ${SRCS})

target_link_libraries(tests leap)


#
# install
#

INSTALL(TARGETS tests DESTINATION bin)

