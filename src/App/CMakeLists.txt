
include_directories("${CMAKE_SOURCE_DIR}")
include_directories("../../")
set( APP_SRCS         "App.cpp"

            )

ADD_LIBRARY(app SHARED ${APP_SRCS})
ADD_LIBRARY(app_st  ${APP_SRCS})

#TARGET_LINK_LIBRARIES(model  lsdynareader ${VTK_LIBRARIES} ${GMSH_LIBS})
#IF (WIN32)
TARGET_LINK_LIBRARIES(app  graphicmesh_st )

TARGET_LINK_LIBRARIES(app_st  graphicmesh_st )
#ELSE()
#TARGET_LINK_LIBRARIES(app  model )
#ENDIF()