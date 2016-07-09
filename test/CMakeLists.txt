#
# libleap test
#

if(POLICY CMP0037)
  cmake_policy(SET CMP0037 OLD)
endif(POLICY CMP0037)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-parameter")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")

if(MINGW)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mthreads")
endif(MINGW)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -ffast-math")

include_directories(${PROJECT_SOURCE_DIR}/include)

set(SRCS ${SRCS} main.cpp)
set(SRCS ${SRCS} vector.cpp)
set(SRCS ${SRCS} matrix.cpp)
set(SRCS ${SRCS} swizzle.cpp)
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

add_executable(test ${SRCS})

target_link_libraries(test leap)


#
# install
#

INSTALL(TARGETS test DESTINATION bin)

