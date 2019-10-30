set(TCHECKER_EXT_BINARY_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/../include")

include_directories(${TCHECKER_INCLUDE_DIR})
include_directories(${TCHECKER_EXT_INCLUDE_DIR})
include_directories(${TCHECKER_EXT_BINARY_INCLUDE_DIR})
link_directories(${TCHECKER_LINK_DIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

if(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Linux")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

add_subdirectory(algorithms/explore_ext)
add_subdirectory(algorithms/covreach_ext)
add_subdirectory(post_proc)
add_subdirectory(dbm)
add_subdirectory(utils)

set(LIBTCHECKER_EXT_SRC
${EXPLORE_EXT_SRC}
${COVREACH_EXT_SRC}
${DBM_EXT_SRC}
${UTILS_EXT_SRC}
${POST_PROC_SRC}
)

#add_library(tchecker_orig_lib STATIC IMPORTED)
#set_property(TARGET tchecker_orig_lib PROPERTY IMPORTED_LOCATION "${TCHECKER_LINK_DIR}/libtchecker.a")

# Build extend TChecker static library
add_library(libtchecker_ext_static STATIC ${LIBTCHECKER_EXT_SRC})
target_link_libraries(libtchecker_ext_static libtchecker_static)
set_property(TARGET libtchecker_ext_static PROPERTY OUTPUT_NAME tchecker_ext_lib)
set_property(TARGET libtchecker_ext_static PROPERTY CXX_STANDARD 17)
set_property(TARGET libtchecker_ext_static PROPERTY CXX_STANDARD_REQUIRED ON)

# Build extended TChecker executable
add_executable(tchecker_ext ${CMAKE_CURRENT_SOURCE_DIR}/tchecker_ext/tchecker_ext.cc)
target_link_libraries(tchecker_ext libtchecker_ext_static)
set_property(TARGET tchecker_ext PROPERTY CXX_STANDARD 17)
set_property(TARGET tchecker_ext PROPERTY CXX_STANDARD_REQUIRED ON)

# Build extended TChecker executable
add_executable(swaparidu ${CMAKE_CURRENT_SOURCE_DIR}/tchecker_ext/test_swap.cc)
target_link_libraries(swaparidu libtchecker_ext_static)
set_property(TARGET swaparidu PROPERTY CXX_STANDARD 17)
set_property(TARGET swaparidu PROPERTY CXX_STANDARD_REQUIRED ON)

## Build extended TChecker executable
#add_executable(explore_mono ${CMAKE_CURRENT_SOURCE_DIR}/tchecker_ext/explore_mono.cc)
#target_link_libraries(explore_mono libtchecker_ext_static)
#set_property(TARGET explore_mono PROPERTY CXX_STANDARD 17)
#set_property(TARGET explore_mono PROPERTY CXX_STANDARD_REQUIRED ON)
#
#add_executable(test_cpy ${CMAKE_CURRENT_SOURCE_DIR}/tchecker_ext/test_cpy.cc)
#target_link_libraries(test_cpy libtchecker_static)
#set_property(TARGET test_cpy PROPERTY CXX_STANDARD 17)
#set_property(TARGET test_cpy PROPERTY CXX_STANDARD_REQUIRED ON)

#add_executable(test_dbm ${CMAKE_CURRENT_SOURCE_DIR}/tchecker_ext/test_dbm.cc)
#target_link_libraries(test_dbm libtchecker_static)
#set_property(TARGET test_dbm PROPERTY CXX_STANDARD 17)
#set_property(TARGET test_dbm PROPERTY CXX_STANDARD_REQUIRED ON)