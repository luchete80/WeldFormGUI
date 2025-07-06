
include_directories("${CMAKE_SOURCE_DIR}")
include_directories("../../")
include_directories("../io")
include_directories("../geom")
include_directories("${CMAKE_SOURCE_DIR}/io")
set( MODEL_SRCS         "BoundaryCondition.cpp" 
                        "Entity.cpp"
                        "Mesh.cpp"
                        "SPHModel.cpp"
                        "Element.cpp"
                        #"Geom.cpp"
                               "Model.cpp"
                               "SPHMesh.cpp"
                               "Part.cpp"            )

ADD_LIBRARY(model SHARED ${MODEL_SRCS})

#ADD_LIBRARY(model_st ${MODEL_SRCS})
#TARGET_LINK_LIBRARIES(model  lsdynareader ${VTK_LIBRARIES} ${GMSH_LIBS})
TARGET_LINK_LIBRARIES(model  lsdynareader gmsh geom)

#TARGET_LINK_LIBRARIES(model_st  lsdynareader gmsh)

# Set destination dir
set(LIB_OUTPUT_DIR ${CMAKE_BINARY_DIR}/src)

add_custom_command(TARGET model POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:model>
        ${LIB_OUTPUT_DIR}/$<TARGET_FILE_NAME:model>
    COMMENT "Copying model to bin/"
)