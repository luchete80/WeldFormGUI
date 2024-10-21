
include_directories("${CMAKE_SOURCE_DIR}")
include_directories("../../")
set( APP_SRCS         "App.cpp"

            )

ADD_LIBRARY(app SHARED ${APP_SRCS})

#TARGET_LINK_LIBRARIES(model  lsdynareader ${VTK_LIBRARIES} ${GMSH_LIBS})
TARGET_LINK_LIBRARIES(app  model )
