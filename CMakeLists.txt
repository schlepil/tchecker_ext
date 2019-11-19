cmake_minimum_required(VERSION 3.13)
project(tchecker_ext)

set(CMAKE_CXX_STANDARD 17)

include("${CMAKE_CURRENT_SOURCE_DIR}/tchecker/cmake/CheckCompilerFlags.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/CheckCompilerFlags.cmake")

set(TCHECKER_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/tchecker/")

#TODO change the build structure
set(TCHECKER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tchecker/)
set(TCHECKER_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tchecker/include ${TCHECKER_BUILD_DIR}/include)
set(TCHECKER_LINK_DIR ${TCHECKER_BUILD_DIR}/src)
set(TCHECKER_EXT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

tck_check_cxx_flags("-fstrict-vtable-pointers" STRICT_VTABLE_POINTERS)
tck_check_cxx_flags("-ferror-limit=10" ERROR_LIMIT)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${STRICT_VTABLE_POINTERS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g ${ERROR_LIMIT}")

add_subdirectory(include)

include_directories(${TCHECKER_EXT_INCLUDE_DIR})

add_subdirectory(${TCHECKER_DIR})

add_subdirectory(src)

