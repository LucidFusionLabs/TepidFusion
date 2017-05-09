# $Id: CMakeLists.txt 1325 2014-10-29 06:08:31Z justin $

# libuv
if(LFL_LIBUV)
  ExternalProject_Add(uv LOG_CONFIGURE ON LOG_BUILD ON
                      PREFIX ${CMAKE_CURRENT_BINARY_DIR}/libuv
                      SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libuv
                      CONFIGURE_COMMAND <SOURCE_DIR>/autogen.sh --prefix=<INSTALL_DIR>
                      COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>)
  add_library(libuv IMPORTED STATIC GLOBAL)
  add_dependencies(libuv uv)
  set_property(TARGET libuv PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/libuv/lib/libuv.a)
  set(LIBUV_INCLUDE ${CMAKE_CURRENT_BINARY_DIR}/libuv/include PARENT_SCOPE)
  set(LIBUV_LIB libuv PARENT_SCOPE)
endif()

# cmake-daemon
if(LFL_CMAKEDAEMON)
  ExternalProject_Add(cmake-daemon LOG_CONFIGURE ON LOG_BUILD ON
                      PREFIX ${CMAKE_CURRENT_BINARY_DIR}/cmake-daemon
                      SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cmake-daemon
                      CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_CURRENT_BINARY_DIR}/cmake-daemon
                      -DLIBUV_INCLUDE_DIR=${CMAKE_CURRENT_BINARY_DIR}/libuv/include
                      -DLIBUV_LIBRARY=${CMAKE_CURRENT_BINARY_DIR}/libuv/lib/libuv.a
                      -DDTL_INCLUDE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/dtl)
  add_dependencies(cmake-daemon libuv)
  add_executable(cmakedaemon IMPORTED GLOBAL)
  add_dependencies(cmakedaemon cmake-daemon)
  set_property(TARGET cmakedaemon PROPERTY IMPORTED_LOCATION
               ${CMAKE_CURRENT_BINARY_DIR}/cmake-daemon/bin/cmake)
endif()
