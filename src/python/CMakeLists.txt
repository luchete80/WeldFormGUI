

INCLUDE(${SWIG_USE_FILE})

INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})

FILE(GLOB SRCS *.h *.cpp)
FILE(GLOB ISRCS *.i)

SET(CMAKE_SWIG_FLAGS "-Wall")

MESSAGE ("INCLUDE DIR" ${CMAKE_SOURCE_DIR}/libs/imgui)
SET(SWINCFLAGS 
-I${PROJECT_SOURCE_DIR}/src/common/math
-I${CMAKE_SOURCE_DIR}/libs/imgui
-I${CMAKE_SOURCE_DIR}/libs/glfw-3.3.4/include/GLFW

)

SET_SOURCE_FILES_PROPERTIES(${ISRCS} PROPERTIES SWIG_FLAGS "${SWINCFLAGS}")
SET_SOURCE_FILES_PROPERTIES(${ISRCS} PROPERTIES CPLUSPLUS ON)

 

#SWIG_ADD_LIBRARY(pyglobal TYPE STATIC LANGUAGE python SOURCES ${ISRCS} ${SRCS})

SWIG_ADD_LIBRARY(dnlPython  LANGUAGE python SOURCES ${ISRCS} ${SRCS})

MESSAGE ("INCLUDE DIR" ${CMAKE_SOURCE_DIR}/libs/glfw-3.3.4/include/GLFW)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src/common/math)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/libs/imgui)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/libs/glfw-3.3.4/include/GLFW)


#if (WIN32)
#SWIG_LINK_LIBRARIES(dnlPython app model_st ${PYTHON_LIBRARIES} ${ZLIB_LIBRARIES}             
                                    
                                   # ${GMSH_LIBS}) 
#else()
SWIG_LINK_LIBRARIES(dnlPython app model ${PYTHON_LIBRARIES} ${ZLIB_LIBRARIES}             
                                    
                                    ${GMSH_LIBS}) 
#endif()
#ADD_CUSTOM_COMMAND(TARGET _dnlPython POST_BUILD
#                   COMMAND ${CMAKE_COMMAND} -E copy_if_different
#                   ${CMAKE_BINARY_DIR}/dnlPython/dnlPython.py
#                   ${EXECUTABLE_OUTPUT_PATH})

SOURCE_GROUP(base             REGULAR_EXPRESSION ".*\\.(C|i|h)")
SOURCE_GROUP(generated        REGULAR_EXPRESSION ".*\\.(cxx|py)")
