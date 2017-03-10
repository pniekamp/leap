#
# libleap
#

if(UNIX OR MINGW)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wfloat-conversion -Wno-unused-parameter -Wno-unused-function")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wfloat-conversion -Wno-unused-parameter -Wno-unused-function")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -ffast-math")
endif(UNIX OR MINGW)

if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4244 /wd4800 /wd4267")
endif(MSVC)

if(MINGW)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mthreads")
endif(MINGW)

if(UNIX)
  add_definitions(-fpic)
endif(UNIX)

if(WIN32)
  add_definitions(-D_WIN32_WINNT=0x0600 -DWIN32_LEAN_AND_MEAN -DNOMINMAX -D_CRT_SECURE_NO_WARNINGS -D_WINSOCK_DEPRECATED_NO_WARNINGS)
endif(WIN32)

include_directories(${PROJECT_SOURCE_DIR}/include)

set(SRCS ${SRCS} ../include/leap.h)
set(SRCS ${SRCS} ../include/leap/util.h)
set(SRCS ${SRCS} ../include/leap/optional.h)
set(SRCS ${SRCS} ../include/leap/sapstream.h)
set(SRCS ${SRCS} ../include/leap/siglib.h)
set(SRCS ${SRCS} ../include/leap/sockets.h)
set(SRCS ${SRCS} ../include/leap/concurrentqueue.h)
set(SRCS ${SRCS} ../include/leap/regex.h regex.cpp)
set(SRCS ${SRCS} ../include/leap/sockets.h sockets.cpp)
set(SRCS ${SRCS} ../include/leap/pathstring.h pathstring.cpp)
set(SRCS ${SRCS} ../include/leap/http.h http.cpp)

if(UNIX)
  set(SRCS ${SRCS} ../include/leap/threadcontrol.h threadcontrol-linux.cpp)
endif(UNIX)

if(WIN32)
  set(SRCS ${SRCS} ../include/leap/threadcontrol.h threadcontrol-win32.cpp)
endif(WIN32)

set(SRCS ${SRCS} ../include/leap/lml/lml.h)
set(SRCS ${SRCS} ../include/leap/lml/io.h)
set(SRCS ${SRCS} ../include/leap/lml/vector.h)
set(SRCS ${SRCS} ../include/leap/lml/matrix.h)
set(SRCS ${SRCS} ../include/leap/lml/simplematrix.h)
set(SRCS ${SRCS} ../include/leap/lml/matrixconstants.h)
set(SRCS ${SRCS} ../include/leap/lml/quaternion.h)
set(SRCS ${SRCS} ../include/leap/lml/interpolation.h)
set(SRCS ${SRCS} ../include/leap/lml/bezier.h)
set(SRCS ${SRCS} ../include/leap/lml/cubicspline.h)
set(SRCS ${SRCS} ../include/leap/lml/evaluator.h evaluator.cpp)
set(SRCS ${SRCS} ../include/leap/lml/point.h)
set(SRCS ${SRCS} ../include/leap/lml/geometry.h)
set(SRCS ${SRCS} ../include/leap/lml/geometry2d.h)
set(SRCS ${SRCS} ../include/leap/lml/geometry3d.h)
set(SRCS ${SRCS} ../include/leap/lml/polygonsetop_p.h)
set(SRCS ${SRCS} ../include/leap/lml/polygonsimplify_p.h)
set(SRCS ${SRCS} ../include/leap/lml/delaunay2d_p.h)
set(SRCS ${SRCS} ../include/leap/lml/voronoi2d_p.h)
set(SRCS ${SRCS} ../include/leap/lml/bound.h)
set(SRCS ${SRCS} ../include/leap/lml/rtree.h)
set(SRCS ${SRCS} ../include/leap/lml/rtreesearch.h)
set(SRCS ${SRCS} ../include/leap/lml/octree.h)
set(SRCS ${SRCS} ../include/leap/lml/array.h)

set(SRCS ${SRCS} ../include/leap/lz4.h lz4.cpp)
set(SRCS ${SRCS} ../include/leap/hash.h hash-md5.cpp hash-sha1.cpp)

add_library(leap STATIC ${SRCS})

target_link_libraries(leap ${CMAKE_THREAD_LIBS_INIT})

if(WIN32)
  target_link_libraries(leap WS2_32)
endif(WIN32)


#
# install
#

INSTALL(TARGETS leap DESTINATION lib)

