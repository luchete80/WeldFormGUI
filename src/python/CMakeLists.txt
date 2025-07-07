

INCLUDE(${SWIG_USE_FILE})

INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
include_directories(${PYTHON_INCLUDE_DIRS})

FILE(GLOB SRCS Dummy.h *.h *.cpp)
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
                                    
                                    ${GMSH_LIBS} io geom) 
#endif()
#ADD_CUSTOM_COMMAND(TARGET _dnlPython POST_BUILD
#                   COMMAND ${CMAKE_COMMAND} -E copy_if_different
#                   ${CMAKE_BINARY_DIR}/dnlPython/dnlPython.py
#                   ${EXECUTABLE_OUTPUT_PATH})

SOURCE_GROUP(base             REGULAR_EXPRESSION ".*\\.(C|i|h)")
SOURCE_GROUP(generated        REGULAR_EXPRESSION ".*\\.(cxx|py)")

set(PYTHON_WRAPPER_OUTPUT_DIR ${CMAKE_BINARY_DIR})

# Copy the Python extension and .py wrapper after build
add_custom_command(TARGET _dnlPython POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "${PYTHON_WRAPPER_OUTPUT_DIR}"
  # Copy the compiled Python extension module (.so or .pyd)
  COMMAND ${CMAKE_COMMAND} -E copy
      $<TARGET_FILE:_dnlPython>
      ${PYTHON_WRAPPER_OUTPUT_DIR}

  # Copy the SWIG-generated Python wrapper
  COMMAND ${CMAKE_COMMAND} -E copy
      ${CMAKE_CURRENT_BINARY_DIR}/dnlPython.py
      ${PYTHON_WRAPPER_OUTPUT_DIR}

  # Copy your own model.py file
  COMMAND ${CMAKE_COMMAND} -E copy
      ${CMAKE_CURRENT_BINARY_DIR}/model.py
      ${PYTHON_WRAPPER_OUTPUT_DIR}
  COMMENT "Copying SWIG Python wrapper (.py) and extension module (.pyd/.so) to python folder"
)

