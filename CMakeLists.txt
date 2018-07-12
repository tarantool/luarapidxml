cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(luarapidxml)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

# Find Tarantool and Lua dependecies
set(TARANTOOL_FIND_REQUIRED ON)

find_package(Tarantool)
include_directories(${TARANTOOL_INCLUDE_DIRS})

if (APPLE)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -undefined suppress -flat_namespace")
endif(APPLE)

# Add C library
include_directories(src/)
add_library(luarapidxml SHARED src/luarapidxml.cpp)

if (NOT APPLE)
  target_link_libraries(luarapidxml -static-libgcc -static-libstdc++)
endif(NOT APPLE)

# target_link_libraries(lib ${MSGPUCK_LIBRARIES})
set_target_properties(luarapidxml PROPERTIES PREFIX "")

# Install module
install(TARGETS luarapidxml LIBRARY
    DESTINATION ${TARANTOOL_INSTALL_LIBDIR}
)

# Testing
set(LUA_CPATH "${CMAKE_BINARY_DIR}/?.so\\;\\;")

enable_testing()

add_test(luarapidxml ${CMAKE_SOURCE_DIR}/test.lua)
file(COPY ${CMAKE_SOURCE_DIR}/fixtures/ DESTINATION fixtures/)
set_property(TEST luarapidxml PROPERTY ENVIRONMENT "LUA_CPATH=${LUA_CPATH}")
