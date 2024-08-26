#THESE ARE FOR IMGUI-VTK
set (IMGUI_SRC_DIR ${PROJECT_SOURCE_DIR}/libs/imgui-1.87)
set (GL3W_SOURCE_DIR ${PROJECT_SOURCE_DIR}/libs/gl3w)
include_directories("${CMAKE_SOURCE_DIR}/libs/glad/include")

add_subdirectory("freetype-2.10.4")
add_subdirectory("glfw-3.3.4")
add_subdirectory("LSDynaReader/src")
add_subdirectory("gmsh")
add_subdirectory("gl3w")
#add_subdirectory("imgui-vtk")


