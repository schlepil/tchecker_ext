# This file is a part of the TChecker project.
#
# See files AUTHORS and LICENSE for copyright details.

set(COVREACH_EXT_SRC
${CMAKE_CURRENT_SOURCE_DIR}/options.cc
${CMAKE_CURRENT_SOURCE_DIR}/run.cc
${CMAKE_CURRENT_SOURCE_DIR}/stats.cc
#${TCHECKER_EXT_INCLUDE_DIR}/tchecker/algorithms/covreach/accepting.hh
${TCHECKER_EXT_INCLUDE_DIR}/tchecker_ext/algorithms/covreach_ext/algorithm.hh
#${TCHECKER_EXT_INCLUDE_DIR}/tchecker/algorithms/covreach/builder.hh
#${TCHECKER_EXT_INCLUDE_DIR}/tchecker/algorithms/covreach/cover.hh
${TCHECKER_EXT_INCLUDE_DIR}/tchecker_ext/algorithms/covreach_ext/graph.hh
${TCHECKER_EXT_INCLUDE_DIR}/tchecker_ext/algorithms/covreach_ext/options.hh
#${TCHECKER_EXT_INCLUDE_DIR}/tchecker/algorithms/covreach/output.hh
${TCHECKER_EXT_INCLUDE_DIR}/tchecker_ext/algorithms/covreach_ext/run.hh
${TCHECKER_EXT_INCLUDE_DIR}/tchecker_ext/algorithms/covreach_ext/stats.hh
${TCHECKER_EXT_INCLUDE_DIR}/tchecker_ext/algorithms/covreach_ext/waiting.hh
PARENT_SCOPE)
