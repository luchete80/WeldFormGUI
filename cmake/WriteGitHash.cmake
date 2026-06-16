execute_process(
  COMMAND git rev-parse --short HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
)

execute_process(
  COMMAND git describe --tags
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_DESCRIBE
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
)

if ("${GIT_HASH}" STREQUAL "")
  set(GIT_HASH "unknown")
endif()

if ("${GIT_DESCRIBE}" STREQUAL "")
  set(GIT_DESCRIBE "${VERSION}")
endif()

file(WRITE ${OUT_HEADER} "// Auto-generated file - do not edit\n")
file(APPEND ${OUT_HEADER} "#pragma once\n")
file(APPEND ${OUT_HEADER} "#define GIT_COMMIT_HASH \"${GIT_HASH}\"\n")
file(APPEND ${OUT_HEADER} "#define PROJECT_VERSION \"${VERSION}\"\n")
file(APPEND ${OUT_HEADER} "#define GIT_DESCRIBE_VERSION \"${GIT_DESCRIBE}\"\n")
string(TIMESTAMP BUILD_DATE "%Y-%m-%d")
string(TIMESTAMP BUILD_TIME "%H:%M:%S")
string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%d %H:%M:%S")
file(APPEND ${OUT_HEADER} "#define BUILD_DATE \"${BUILD_DATE}\"\n")
file(APPEND ${OUT_HEADER} "#define BUILD_TIME \"${BUILD_TIME}\"\n")
file(APPEND ${OUT_HEADER} "#define BUILD_TIMESTAMP \"${BUILD_TIMESTAMP}\"\n")
