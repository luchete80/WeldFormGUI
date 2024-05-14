
include_directories("${CMAKE_SOURCE_DIR}")

FILE(GLOB_RECURSE MODEL_SRCS         "*.h" "Mesh.cpp"
                                           "gMesh.cpp" #graphic instance of gmsh
                                           )

ADD_LIBRARY(model ${MODEL_SRCS})