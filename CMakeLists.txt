cmake_minimum_required(VERSION 2.4.3) #TODO: CHECK

include_directories("${CMAKE_SOURCE_DIR}")
include_directories("${CMAKE_SOURCE_DIR}/src")
include_directories("${CMAKE_SOURCE_DIR}/freeglut-3.2.1/include")
include_directories("${CMAKE_SOURCE_DIR}/glew-cmake-2.2.0/include")
include_directories("${CMAKE_SOURCE_DIR}/Common")
include_directories("${CMAKE_SOURCE_DIR}/Include")
include_directories("${CMAKE_SOURCE_DIR}/include_logl")
include_directories("${CMAKE_SOURCE_DIR}/glad/include")
include_directories("${CMAKE_SOURCE_DIR}/configuration")

#libs
# ########### GAME ########
add_subdirectory("src")


#renderer
#add_subdirectory("OpenGLWindow")
add_subdirectory("libs")
add_subdirectory("examples")
#add_subdirectory("tests")
