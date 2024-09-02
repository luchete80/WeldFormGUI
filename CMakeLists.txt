cmake_minimum_required(VERSION 2.4.3) #TODO: CHECK

include_directories("${CMAKE_SOURCE_DIR}")
include_directories("${CMAKE_SOURCE_DIR}/src")
include_directories("${CMAKE_SOURCE_DIR}/freeglut-3.2.1/include")
include_directories("${CMAKE_SOURCE_DIR}/glew-cmake-2.2.0/include")
include_directories("${CMAKE_SOURCE_DIR}/common")
include_directories("${CMAKE_SOURCE_DIR}/Include")
include_directories("${CMAKE_SOURCE_DIR}/include_logl")
include_directories("${CMAKE_SOURCE_DIR}/glad/include")
include_directories("${CMAKE_SOURCE_DIR}/configuration")
include_directories("${CMAKE_SOURCE_DIR}/libs")

include_directories("${CMAKE_SOURCE_DIR}/libs/")
include_directories("${CMAKE_SOURCE_DIR}/libs/json")
include_directories("${CMAKE_SOURCE_DIR}/libs/gmsh/")
include_directories("${CMAKE_SOURCE_DIR}/libs/gmsh/api")
include_directories("${CMAKE_SOURCE_DIR}/libs/gmsh/src/common")
include_directories("${CMAKE_SOURCE_DIR}/libs/gmsh/src/geo")
include_directories("${CMAKE_SOURCE_DIR}/libs/gmsh/src/graphics")
include_directories("${CMAKE_SOURCE_DIR}/libs/gmsh/src/mesh")
include_directories("${CMAKE_SOURCE_DIR}/libs/gmsh/src/numeric")

include_directories("${VTK_INCLUDE_DIR}")
set (gl3w_dir "${CMAKE_SOURCE_DIR}/libs/gl3w")
set (imgui_dir "${CMAKE_SOURCE_DIR}/libs/imgui") ###NOT THE 1.87 ONE, WHICH FAILS!
#libs
# ########### GAME ########
add_subdirectory("libs")
add_subdirectory("src")
add_subdirectory("tests")



#renderer
#add_subdirectory("OpenGLWindow")

#add_subdirectory("tests")

