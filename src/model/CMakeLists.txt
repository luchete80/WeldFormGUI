
include_directories("${CMAKE_SOURCE_DIR}")
include_directories("../../")
set( MODEL_SRCS         "Entity.cpp"
                        "Mesh.cpp"
                        "SPHModel.cpp"
                        "Element.cpp"
                        "Geom.cpp"
                               "Model.cpp"
                               "Part.cpp"            )

ADD_LIBRARY(model SHARED ${MODEL_SRCS})

ADD_LIBRARY(model_st ${MODEL_SRCS})
#TARGET_LINK_LIBRARIES(model  lsdynareader ${VTK_LIBRARIES} ${GMSH_LIBS})
TARGET_LINK_LIBRARIES(model  lsdynareader gmsh)

TARGET_LINK_LIBRARIES(model_st  lsdynareader gmsh)