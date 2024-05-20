
include_directories("${CMAKE_SOURCE_DIR}")

FILE(GLOB_RECURSE MODEL_SRCS         "*.h" "Mesh.cpp"
                                           "Element.cpp"
                                           "Model.cpp"
                                           )

ADD_LIBRARY(model ${MODEL_SRCS})

TARGET_LINK_LIBRARIES(model ${MODEL_SRCS} lsdynareader)