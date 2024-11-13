
include_directories("${CMAKE_SOURCE_DIR}")
include_directories("../../")

if (WIN32) #WIH CAPS ON!
MESSAGE ("----------------------------THIS IS WIN")
set( APP_SRCS         "App.cpp"
                      "../graphicmesh/GraphicMesh.cpp"
            )
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

else()
set( APP_SRCS         "App.cpp"

            )
endif()


include_directories("${VTK_INCLUDE_DIR}")


#ADD_LIBRARY(app_st  ${APP_SRCS})

#TARGET_LINK_LIBRARIES(model  lsdynareader ${VTK_LIBRARIES} ${GMSH_LIBS})
IF (WIN32)
ADD_LIBRARY(app OBJECT ${APP_SRCS})
TARGET_LINK_LIBRARIES(app   model ${VTK_LIBRARIES} gmsh)

#TARGET_LINK_LIBRARIES(app_st  graphicmesh_st )
ELSE()
ADD_LIBRARY(app SHARED ${APP_SRCS})
TARGET_LINK_LIBRARIES(app  model graphicmesh_)
ENDIF()
