# Setup build environment
set(TARGET rbdl)

#find_package(PkgConfig)
#pkg_check_modules(RBDL rbdl)
# 
#if (RBDL_FOUND)
#  find_library(LUA_ADDON_LIB NAMES rbdl_luamodel HINTS ${RBDL_LIBRARY_DIRS})
#  find_path(LUA_ADDON_INC rbdl/addons/luamodel/luamodel.h ${RBDL_INCLUDE_DIRS})
#
#  if (LUA_ADDON_LIB AND LUA_ADDON_INC)
#    message("-- Using external RBDL library")
#    include_directories(${RBDL_INCLUDE_DIRS})
#    return()
#  endif()
#endif()
#
#message("-- Building included RBDL library")

FIND_PACKAGE (Eigen3 3.0.0)
IF (EIGEN3_FOUND)
  INCLUDE_DIRECTORIES (${EIGEN3_INCLUDE_DIR})
ENDIF ()

execute_process(
  COMMAND rm -rf externals/rbdl
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

execute_process(
  COMMAND ${CMAKE_COMMAND} -E tar xf ${SRC}/../share/default.zip
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

execute_process(
  COMMAND mkdir -p externals
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

execute_process(
  COMMAND mv rbdl-rbdl-de94c4fadf94 externals/rbdl
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

execute_process(
  COMMAND mkdir externals/rbdl/build
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

OPTION (RBDL_BUILD_ADDON_LUAMODEL "Build the lua model reader" ON)
set(RBDL_BUILD_ADDON_LUAMODEL ON CACHE BOOL FORCE "Build the lua model reader")

add_subdirectory(${CMAKE_BINARY_DIR}/externals/rbdl ${CMAKE_BINARY_DIR}/externals/rbdl/build)

include_directories(${CMAKE_BINARY_DIR}/externals)
include_directories(${CMAKE_BINARY_DIR}/externals/rbdl/include)
include_directories(${CMAKE_BINARY_DIR}/externals/rbdl/build/include)
