cmake_minimum_required(VERSION 2.8)

project(graphics)

# find_package(ImageMagick COMPONENTS Magick++)

# set (ImageMagick_INCLUDE_DIRS "D:/Luciano/ImageMagick-7")

# include_directories("d:/Luciano/ImageMagick-7")
# link_directories("d:/Luciano/ImageMagick-7/lib")
# if( MINGW )
    set(glut freeglut)
    if (WIN32)
    set(GL openGL32)
    elseif (UNIX)
    
     find_package(OpenGL REQUIRED)
  add_definitions(${OPENGL_DEFINITIONS})
  find_package(X11 REQUIRED) 
    set(GL GL X11)
    endif()
    set(GLEW GLEW32)

    # include_directories(../Include ${ImageMagick_INCLUDE_DIRS})

    # link_libraries(glut glfw assimp AntTweakBar)
# else()
    # set(glut glut)
    # set(GL GL)
    # set(GLEW GLEW)

    # find_package(PkgConfig REQUIRED)

    # pkg_search_module(GLFW REQUIRED glfw3)
	include_directories("${CMAKE_SOURCE_DIR}/Libs/freeglut-mingw/include")	
	include_directories("${CMAKE_SOURCE_DIR}/Libs/glfw-3.3.4/include")
	include_directories("${CMAKE_SOURCE_DIR}/Libs/assimp-3.1/include")
	
    include_directories(../../include/ogldev ${ImageMagick_INCLUDE_DIRS} ${GLFW_INCLUDE_DIRS})
	
    # link_libraries(glut ${GLFW_LIBRARIES} assimp AntTweakBar)
# endif()

link_directories	("d:/Luciano/glfw-3.3.4/bin/src")
link_directories	("D:/Luciano/assimp-3.1_bin/code")

add_library(math_3d  math_3d.cpp)
add_library(pipeline  pipeline.cpp)
target_link_libraries( pipeline math_3d)
add_library(backend  ogldev_backend.cpp 	

							ogldev_glfw_backend.cpp 
							${CMAKE_SOURCE_DIR}/libs/glad/src/glad.c
							)
target_link_libraries( backend util 
						glfw3
						#${GLEW} 
						${GL})
 
#add_library(io_buffer SHARED io_buffer.cpp)
add_library(util  ogldev_util.cpp)
add_library(basic_lighting ogldev_basic_lighting.cpp 
							${CMAKE_SOURCE_DIR}/libs/glad/src/glad.c
			)
      
#add_library (shapes shapes.cpp)

add_library(shadow_map_fbo #SHARED 
							ogldev_shadow_map_fbo.cpp)

add_library(app  ogldev_app.cpp)
target_link_libraries(app util)

link_directories ("D:/Luciano/Libs/freeglut-mingw/lib/x64")
# add_library(atb SHARED ogldev_atb.cpp)

add_library(texture 
				#SHARED 
				ogldev_texture.cpp 
				#random_texture.cpp 
				#cubemap_texture.cpp 
				${CMAKE_SOURCE_DIR}/libs/glad/src/glad.c)
target_link_libraries(texture 

${GL}
)

add_library(camera  camera.cpp arcball_camera.cpp) #TODO: delete camera.cpp
target_link_libraries(camera math_3d)


add_library(technique technique.cpp
							${CMAKE_SOURCE_DIR}/libs/glad/src/glad.c
							)
target_link_libraries(technique util ${GL})

target_link_libraries(basic_lighting
						technique
						math_3d)
