cmake_minimum_required(VERSION 3.13)
project(tchecker_ext)

set(CMAKE_CXX_STANDARD 17)

include("${CMAKE_CURRENT_SOURCE_DIR}/tchecker/cmake/CheckCompilerFlags.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/CheckCompilerFlags.cmake")

#set(TCHECKER_BUILD_DIR "/home/philipp/temp/tchecker_fork_build")
set(TCHECKER_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/tchecker/")

#TODO change the build structure
set(TCHECKER_DIR ./tchecker/)
set(TCHECKER_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tchecker/include ${TCHECKER_BUILD_DIR}/include)
set(TCHECKER_LINK_DIR ${TCHECKER_BUILD_DIR}/src)
set(TCHECKER_EXT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCMAKE_CXX_COMPILER=clang++-8 -DCMAKE_C_COMPILER=clang-8")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -fPIC")

tck_check_cxx_flags("-fstrict-vtable-pointers" STRICT_VTABLE_POINTERS)
tck_check_cxx_flags("-ferror-limit=10" ERROR_LIMIT)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fmax-errors=10")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fPIC")
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -flto ${STRICT_VTABLE_POINTERS}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${STRICT_VTABLE_POINTERS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g ${ERROR_LIMIT}")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCMAKE_CXX_COMPILER=g++ -fPIC")

add_subdirectory(${TCHECKER_DIR})

add_subdirectory(include)
add_subdirectory(src)

