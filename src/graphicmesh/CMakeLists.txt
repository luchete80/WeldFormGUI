if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
    set(BUILD_SHARED_LIBS TRUE)
endif()


include_directories("${CMAKE_SOURCE_DIR}")
include_directories("../../")
set( MESHVIEWER_SRCS         "GraphicMesh.cpp" )

message (STATUS "VTK Version: ${VTK_VERSION}")
message ("VTK LIBS:" ${VTK_LIBRARIES})

include_directories("${VTK_INCLUDE_DIR}")
include_directories(imgui_dir)
include_directories("${CMAKE_SOURCE_DIR}/libs/imgui")
#if (win32)
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

#message ("VTK LIBS:" ${VTK_LIBRARIES})

#endif()

#if (WIN32)
#ADD_LIBRARY(graphicmesh_ ${MESHVIEWER_SRCS})
#"else()
ADD_LIBRARY(graphicmesh_ SHARED ${MESHVIEWER_SRCS})
#endif()
#ADD_LIBRARY(graphicmesh_st ${MESHVIEWER_SRCS})

TARGET_LINK_LIBRARIES(graphicmesh_  model ${VTK_LIBRARIES} gmsh) #GMSH_LIBS WITH ERROR
#TARGET_LINK_LIBRARIES(graphicmesh_st  model ${VTK_LIBRARIES} gmsh) #GMSH_LIBS WITH ERROR

# Set destination dir
set(LIB_OUTPUT_DIR ${CMAKE_BINARY_DIR}/src)

add_custom_command(TARGET graphicmesh_ POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:graphicmesh_>
        ${LIB_OUTPUT_DIR}/$<TARGET_FILE_NAME:graphicmesh_>
    COMMENT "Copying model to bin/"
)
