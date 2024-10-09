
include_directories("${CMAKE_SOURCE_DIR}")
include_directories("../../")
set( MESHVIEWER_SRCS         "GraphicMesh.cpp" )

message (STATUS "VTK Version: ${VTK_VERSION}")
message ("VTK LIBS:" ${VTK_LIBRARIES})

ADD_LIBRARY(graphicmesh SHARED ${MESHVIEWER_SRCS})

TARGET_LINK_LIBRARIES(graphicmesh  ${VTK_LIBRARIES} ${GMSH_LIBS})
