set(TCHECKER_EXT_CONFIG_HH_IN "config.hh.in")
set(TCHECKER_EXT_CONFIG_HH "${CMAKE_CURRENT_BINARY_DIR}/tchecker_ext/config.hh")
set(TCHECKER_EXT_CONFIG_INC_HH "${TCHECKER_EXT_INCLUDE_DIR}/tchecker_ext/config.hh")

configure_file(${TCHECKER_EXT_CONFIG_HH_IN} ${TCHECKER_EXT_CONFIG_HH} @ONLY)
configure_file(${TCHECKER_EXT_CONFIG_HH_IN} ${TCHECKER_EXT_CONFIG_INC_HH} @ONLY)

install(DIRECTORY ../include/ DESTINATION include)
include_directories ("${CMAKE_CURRENT_BINARY_DIR}/tchecker_ext")
