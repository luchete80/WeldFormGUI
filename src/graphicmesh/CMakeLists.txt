
include_directories("${CMAKE_SOURCE_DIR}")
include_directories("../../")
set( MESHVIEWER_SRCS         "MeshViewer.cpp" )

ADD_LIBRARY(graphicmesh SHARED ${MESHVIEWER_SRCS})

TARGET_LINK_LIBRARIES(graphicmesh  lsdynareader ${VTK_LIBRARIES} ${GMSH_LIBS})
