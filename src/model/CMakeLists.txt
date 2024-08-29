
include_directories("${CMAKE_SOURCE_DIR}")
include_directories("../../")
set( MODEL_SRCS         "Mesh.cpp"
                        "Domain.cpp"
                               "Element.cpp"
                               "Model.cpp"
                               "Part.cpp"            )

ADD_LIBRARY(model ${MODEL_SRCS})

TARGET_LINK_LIBRARIES(model  lsdynareader)
