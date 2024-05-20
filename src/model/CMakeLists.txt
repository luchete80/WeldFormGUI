
include_directories("${CMAKE_SOURCE_DIR}")
include_directories("../../")
set( MODEL_SRCS         "Mesh.cpp"
                               "Element.cpp"
                               "Model.cpp"
                                           )

ADD_LIBRARY(model ${MODEL_SRCS})

TARGET_LINK_LIBRARIES(model  lsdynareader)