include_directories("${CMAKE_SOURCE_DIR}/libs/glm-0.9.9.8")
include_directories("${CMAKE_SOURCE_DIR}/libs/glad/include")
include_directories("${CMAKE_SOURCE_DIR}/libs/glm-0.9.9.8")
include_directories("${CMAKE_SOURCE_DIR}/libs/glfw-3.3.4/include")
include_directories("${CMAKE_SOURCE_DIR}/libs/glfw-3.3.4/include")
include_directories("${CMAKE_SOURCE_DIR}/libs/stb_image")
include_directories("${CMAKE_SOURCE_DIR}/libs/assimp-3.1/include")
include_directories("${CMAKE_SOURCE_DIR}/libs/glew-2.2.0/include")
include_directories("${CMAKE_SOURCE_DIR}/libs/freeglut/include")
include_directories("${CMAKE_SOURCE_DIR}/libs/freetype-2.10.4/include")
include_directories("${CMAKE_SOURCE_DIR}/libs/freetype-2.10.4/include/freetype")
include_directories("${CMAKE_SOURCE_DIR}/libs/nlohmann")
include_directories("${CMAKE_SOURCE_DIR}/src/graphics")
include_directories("${CMAKE_SOURCE_DIR}/src/physics")
include_directories("${CMAKE_SOURCE_DIR}/src/model")

include_directories("${CMAKE_SOURCE_DIR}/libs/imgui-1.87")
include_directories("${CMAKE_SOURCE_DIR}/libs/imgui-1.87/backends")
include_directories("${CMAKE_SOURCE_DIR}/libs/ImGuiFileDialog-0.6.4")

include_directories("${CMAKE_SOURCE_DIR}/src/game")
include_directories("${CMAKE_SOURCE_DIR}/src/common")

# link_directories ("D:/Luciano/libs/glfw-3.3.4/src")


# link_directories ("D:/Luciano/libs/freeglut-mingw/lib/x64")
# link_directories ("D:/Luciano/libs/freetype-2.10.4_bin")
link_directories ("${GLFW_BINARY_DIR}/src")

add_subdirectory("graphics")
add_subdirectory("model")

# add_executable(editor 	
							# #camera_system.cpp
							# main.cpp
              # editor_app.cpp
							# ${CMAKE_SOURCE_DIR}/src/mymesh.cpp
							# #skinning_technique.cpp
							# #game.cpp
              # ${CMAKE_SOURCE_DIR}/src/graphics/Renderer.cpp
              # ${CMAKE_SOURCE_DIR}/src/graphics/picking_texture.cpp
              # ${CMAKE_SOURCE_DIR}/src/graphics/picking_technique.cpp	
              
              # ${CMAKE_SOURCE_DIR}/src/text_renderer.cpp
							# ${CMAKE_SOURCE_DIR}/src/shader.cpp
							# ${CMAKE_SOURCE_DIR}/libs/glad/src/glad.c
              
              # ${CMAKE_SOURCE_DIR}/libs/imgui-1.87/imgui.cpp
              # ${CMAKE_SOURCE_DIR}/libs/imgui-1.87/imgui_demo.cpp
              # ${CMAKE_SOURCE_DIR}/libs/imgui-1.87/imgui_draw.cpp
              # ${CMAKE_SOURCE_DIR}/libs/imgui-1.87/imgui_tables.cpp
              # ${CMAKE_SOURCE_DIR}/libs/imgui-1.87/imgui_widgets.cpp
              # ${CMAKE_SOURCE_DIR}/libs/imgui-1.87/backends/imgui_impl_glfw.cpp
              # ${CMAKE_SOURCE_DIR}/libs/imgui-1.87/backends/imgui_impl_opengl3.cpp
              # ${CMAKE_SOURCE_DIR}/libs/ImGuiFileDialog-0.6.4/ImGuiFileDialog.cpp
								# ) 

# target_link_libraries(editor 
            # physics
						# freetype
						# #skinned_mesh
						# basic_mesh
						# basic_lighting
						# #assimp
						# pipeline
						# technique
						# camera
						# shadow_map_fbo
                      # # util
					  # shapes
            # app
					  # backend
					  # glfw3
					  # common
					  # winmm
# )

add_executable(editor 	

              editor.cpp
              ViewportWindow.cpp
              ${CMAKE_SOURCE_DIR}/src/renderer.cpp
							#skinning_technique.cpp
							#game.cpp
              texture.cpp
              model/Domain.cpp
              material_dialog.cpp
              set_dialog.cpp
              #${CMAKE_SOURCE_DIR}/src/graphics/Renderer.cpp
              ${CMAKE_SOURCE_DIR}/src/LSDynaWriter.cpp
              ${CMAKE_SOURCE_DIR}/src/FrameBuffer.cpp
              ${CMAKE_SOURCE_DIR}/src/graphics/ogldev_texture.cpp
              ${CMAKE_SOURCE_DIR}/src/graphics/picking_texture.cpp
              ${CMAKE_SOURCE_DIR}/src/graphics/picking_technique.cpp	
              #${CMAKE_SOURCE_DIR}/src/graphics/renderer.cpp
              
              ${CMAKE_SOURCE_DIR}/src/text_renderer.cpp
							${CMAKE_SOURCE_DIR}/src/shader.cpp
							${CMAKE_SOURCE_DIR}/libs/glad/src/glad.c
  
              
              ${CMAKE_SOURCE_DIR}/libs/imgui-1.87/imgui.cpp
              #${CMAKE_SOURCE_DIR}/libs/imgui-1.87/imgui_demo.cpp
              ${CMAKE_SOURCE_DIR}/libs/imgui-1.87/imgui_draw.cpp    
              ${CMAKE_SOURCE_DIR}/libs/imgui-1.87/imgui_tables.cpp
              ${CMAKE_SOURCE_DIR}/libs/imgui-1.87/imgui_widgets.cpp
              ${CMAKE_SOURCE_DIR}/libs/imgui-1.87/backends/imgui_impl_glfw.cpp
              ${CMAKE_SOURCE_DIR}/libs/imgui-1.87/backends/imgui_impl_opengl3.cpp
              ${CMAKE_SOURCE_DIR}/libs/ImGuiFileDialog-0.6.4/ImGuiFileDialog.cpp
              Job.cpp
              job_dialog.cpp
              log.cpp
              input_writer.cpp
								) 

if (WIN32)
set (LIBS winmm)
endif()

target_link_libraries(editor 
            model
						freetype
						basic_lighting
						pipeline
						technique
						camera
						shadow_map_fbo
                      # util
					  #shapes
            #app
					  backend
					  glfw3
					  #common
					  ${LIBS}
)



# add_executable(editor_app
              # main.cpp
							# #camera_system.cpp
							# editor_app.cpp
							# ${CMAKE_SOURCE_DIR}/src/mymesh.cpp
							# #skinning_technique.cpp
							# #game.cpp
              # ${CMAKE_SOURCE_DIR}/src/graphics/Renderer.cpp
              # ${CMAKE_SOURCE_DIR}/src/graphics/ogldev_texture.cpp
              # ${CMAKE_SOURCE_DIR}/src/graphics/picking_texture.cpp
              # ${CMAKE_SOURCE_DIR}/src/graphics/picking_technique.cpp	
              
              # ${CMAKE_SOURCE_DIR}/src/text_renderer.cpp
							# ${CMAKE_SOURCE_DIR}/src/shader.cpp
							# ${CMAKE_SOURCE_DIR}/libs/glad/src/glad.c
              
              # ${CMAKE_SOURCE_DIR}/libs/imgui-1.87/imgui.cpp
              # ${CMAKE_SOURCE_DIR}/libs/imgui-1.87/imgui_demo.cpp
              # ${CMAKE_SOURCE_DIR}/libs/imgui-1.87/imgui_draw.cpp
              # ${CMAKE_SOURCE_DIR}/libs/imgui-1.87/imgui_tables.cpp
              # ${CMAKE_SOURCE_DIR}/libs/imgui-1.87/imgui_widgets.cpp
              # ${CMAKE_SOURCE_DIR}/libs/imgui-1.87/backends/imgui_impl_glfw.cpp
              # ${CMAKE_SOURCE_DIR}/libs/imgui-1.87/backends/imgui_impl_opengl3.cpp
              # ${CMAKE_SOURCE_DIR}/libs/ImGuiFileDialog-0.6.4/ImGuiFileDialog.cpp
								# ) 
 
# target_link_libraries(editor_app 
            # physics
						# freetype
						# #skinned_mesh
						# basic_mesh
						# basic_lighting
						# #assimp
						# pipeline
						# technique
						# camera
						# shadow_map_fbo
                      # # util
					  # shapes
            # app
					  # backend
					  # glfw3
					  # common
					  # winmm
# )

# add_executable(game_no_bckend main_no_bckend.cpp
							# # camera_system.cpp
							# # game.cpp
							# # mymesh.cpp
							# # text_renderer.cpp
							# # shader.cpp
							# ${CMAKE_SOURCE_DIR}/libs/glad/src/glad.c
								# ) 

# target_link_libraries(game_no_bckend 
            # # physics
						# # freetype
						# # #skinned_mesh
						# # basic_mesh
						# # basic_lighting
						# # #assimp
						# # pipeline
						# # technique
						# # camera
						# # shadow_map_fbo
                      # # # util
					  # # shapes
            # # app
					  # # backend
					  # glfw3
					  # #common
					  # winmm
# )


