
include_directories("${CMAKE_SOURCE_DIR}")

FILE(GLOB_RECURSE MODEL_SRCS         "*.h" "Mesh.cpp")

ADD_LIBRARY(model ${MODEL_SRCS})