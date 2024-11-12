
include_directories("${CMAKE_SOURCE_DIR}")
include_directories("../../")
set( MESHVIEWER_SRCS         "GraphicMesh.cpp" )

message (STATUS "VTK Version: ${VTK_VERSION}")
message ("VTK LIBS:" ${VTK_LIBRARIES})


#SEPARATE FINDING LIBS
find_package(VTK 
  CONFIG REQUIRED 
  COMPONENTS 
  CommonCore
  CommonColor
  CommonDataModel
  FiltersCore
  FiltersSources
  InteractionStyle
  InteractionWidgets
  RenderingAnnotation
  RenderingCore
  RenderingContextOpenGL2
  RenderingFreeType
  RenderingGL2PSOpenGL2
  RenderingOpenGL2
  #DisplayCoordinateAxes
  IOCore
  IOGeometry
  
  IOOCCT
  CommonExecutionModel
  TestingCore
  TestingRendering
  FiltersGeneral
  
  CommonSystem #APP
  vtksys
  QUIET
)

ADD_LIBRARY(graphicmesh SHARED ${MESHVIEWER_SRCS})

ADD_LIBRARY(graphicmesh_st ${MESHVIEWER_SRCS})

TARGET_LINK_LIBRARIES(graphicmesh  model ${VTK_LIBRARIES} gmsh) #GMSH_LIBS WITH ERROR
TARGET_LINK_LIBRARIES(graphicmesh_st  model ${VTK_LIBRARIES} gmsh) #GMSH_LIBS WITH ERROR