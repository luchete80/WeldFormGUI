if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
    set(BUILD_SHARED_LIBS TRUE)
endif()

include_directories("${CMAKE_SOURCE_DIR}")
include_directories("../../")
include_directories("../geom")

if (WIN32) #WIH CAPS ON!
MESSAGE ("----------------------------THIS IS WIN")
set( APP_SRCS         "App.cpp"
                      #"../graphicmesh/GraphicMesh.cpp"
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
#IF (WIN32)

#ADD_LIBRARY(app SHARED ${APP_SRCS})
#target_compile_definitions(app PUBLIC BUILDING_MY_DLL)

#TARGET_LINK_LIBRARIES(app   model ${VTK_LIBRARIES} gmsh)

#TARGET_LINK_LIBRARIES(app_st  graphicmesh_st )
#ELSE()
ADD_LIBRARY(app SHARED ${APP_SRCS})
TARGET_LINK_LIBRARIES(app  model graphicmesh_)
#ENDIF()

# Set destination dir
set(LIB_OUTPUT_DIR ${CMAKE_BINARY_DIR}/src)

add_custom_command(TARGET app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:app>
        ${LIB_OUTPUT_DIR}/$<TARGET_FILE_NAME:app>
    COMMENT "Copying model to bin/"
)
