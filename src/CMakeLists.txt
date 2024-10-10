cmake_minimum_required(VERSION 3.6 FATAL_ERROR)
project(imgui-vtk)

include_directories("${CMAKE_SOURCE_DIR}/src")
include_directories("${CMAKE_SOURCE_DIR}/src/physics")
include_directories("${CMAKE_SOURCE_DIR}/src/model")
include_directories("${CMAKE_SOURCE_DIR}/src/graphicmesh")
include_directories("${CMAKE_SOURCE_DIR}/libs/ImGuiFileDialog-0.6.4")

set(CMAKE_CXX_STANDARD 11)
# set(CMAKE_VERBOSE_MAKEFILE 1)
set(APP_VERSION_MAJOR 1)
set(APP_VERSION_MINOR 0)
set(EXEC_NAME imgui_vtk)

link_directories ("${GLFW_BINARY_DIR}/src")
if (WIN32)
link_directories ("${GMSH_DIR}/lib")
include_directories ("${GMSH_DIR}/include")
endif()


add_subdirectory("model")
add_subdirectory("graphicmesh")
add_subdirectory("io")

# In this CMakeLists.txt file, components are all built separately and linked together
# If you prefer to compile source files + dependencies (OpenGL loaders, imgui, VtkViewer, etc.)
# together, please see CMakeLists-alt.txt
add_executable(${EXEC_NAME} main.cpp 
                            editor.cpp
                            
                            material_dialog.cpp
                            set_dialog.cpp
                            ${CMAKE_SOURCE_DIR}/libs/ImGuiFileDialog-0.6.4/ImGuiFileDialog.cpp
                            Job.cpp
                            job_dialog.cpp
                            log.cpp
                            global.cpp
                            ${CMAKE_SOURCE_DIR}/src/LSDynaWriter.cpp
                            geom/vtkOCCTGeom.cpp
                            
                            #graphics/axis.cpp
                            )
add_library (global_sta STATIC global.cpp)

# OpenGL
find_package(OpenGL REQUIRED)
target_link_libraries(${EXEC_NAME} OpenGL::GL)
if (APPLE)
	# Ignore macOS OpenGL deprecation warnings
	target_compile_definitions(${EXEC_NAME} PRIVATE GL_SILENCE_DEPRECATION)
endif()


target_link_libraries(${EXEC_NAME} gl3w)

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)


# Dear ImGui
#set(imgui_dir ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
file(GLOB imgui_src ${imgui_dir}/*.cpp)
set(imgui_backend ${imgui_dir}/backends/imgui_impl_glfw.cpp ${imgui_dir}/backends/imgui_impl_opengl3.cpp)
add_library(imgui STATIC ${imgui_src} ${imgui_backend})
target_include_directories(imgui PUBLIC ${imgui_dir} ${imgui_dir}/backends)
target_link_libraries(imgui glfw)
target_link_libraries(${EXEC_NAME} imgui)

set (GMSH_LIBS gmsh)
#IF (WIN32)
#  set (GMSH_LIBS gmsh)
#endif()
## https://stackoverflow.com/questions/70790736/application-crashes-when-using-vtksmartvolumemappernew
# VTK
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
  
  QUIET
)

#IOCCT
#DEPENDS
#  VTK::CommonExecutionModel
#PRIVATE_DEPENDS
#  VTK::CommonCore
#  VTK::CommonDataModel
#  VTK::FiltersGeneral
#TEST_DEPENDS
#  VTK::TestingCore
#  VTK::TestingRendering
#  VTK::RenderingOpenGL2
#  VTK::InteractionStyle

if (BUILD_PYTHON)
  # -- Search for Python
  FIND_PACKAGE(PythonInterp 3.8 REQUIRED)
  FIND_PACKAGE(PythonLibs 3.8 REQUIRED)

  # -- Search for Swig
  FIND_PACKAGE(SWIG REQUIRED)
  if (WIN32)
    set (SWIG_DIR D:/Luciano/Numerico/Libs/swigwin-4.1.1/Lib)
  endif()
  add_subdirectory("python")
endif()  
  
if (NOT VTK_FOUND)
  message(FATAL_ERROR "VTK not found!")
  return()
endif()

if (VTK_VERSION VERSION_LESS "9.0.0")
  include(${VTK_USE_FILE})
else()
  # vtk_module_autoinit is needed
  vtk_module_autoinit(
    TARGETS ${EXEC_NAME}
    MODULES ${VTK_LIBRARIES}
  VTK::CommonCore
  VTK::CommonColor
  VTK::CommonDataModel
  VTK::FiltersCore
  VTK::FiltersSources
  VTK::InteractionStyle
  VTK::InteractionWidgets
  VTK::RenderingAnnotation
  VTK::RenderingCore
  VTK::RenderingContextOpenGL2
  VTK::RenderingFreeType
  VTK::RenderingGL2PSOpenGL2
  VTK::RenderingOpenGL2
  VTK::FiltersHybrid
  #DisplayCoordinateAxes
  VTK::IOCore
  VTK::IOGeometry
  VTK::IOOCCT

  VTK::TestingCore
  VTK::TestingRendering
  #VTK::FiltersGeneral
  )
endif()
message (STATUS "VTK Version: ${VTK_VERSION}")
message ("VTK LIBS:" ${VTK_LIBRARIES})

# imgui-vtk (VTK Viewer class)
set(imgui_vtk_viewer_dir ${CMAKE_CURRENT_SOURCE_DIR})
add_library(imgui_vtk_viewer STATIC ${imgui_vtk_viewer_dir}/VtkViewer.cpp

                                    ${imgui_vtk_viewer_dir}/graphics/axis.cpp
                                    )

target_include_directories(imgui_vtk_viewer PUBLIC ${imgui_vtk_viewer_dir})
target_link_libraries(imgui_vtk_viewer gl3w) # Since gl3w was compiled as a static library, we need to link to it
target_link_libraries(imgui_vtk_viewer imgui) # Since imgui was compiled as a static library, we need to link to it
target_link_libraries(imgui_vtk_viewer ${VTK_LIBRARIES} 
                                     
                                      )
#add_library(scene STATIC ${imgui_vtk_viewer_dir}/graphics/axis.cpp)
#target_link_libraries(scene ${VTK_LIBRARIES} gl3w)

target_link_libraries(${EXEC_NAME} imgui_vtk_viewer 
                                  #scene
                                  
                                  )

MESSAGE("LIBRARIES " ${VTK_LIBRARIES})

# GLFW is built from source in this example
# But if you link dynamically, you may need to link some native libraries on macOS:
# target_link_libraries(${EXEC_NAME} "-framework Cocoa" "-framework IOKit" "-framework CoreVideo")
if (UNIX AND NOT APPLE)
  target_link_libraries(${EXEC_NAME} ${CMAKE_THREAD_LIBS_INIT} ${X11_LIBRARIES} ${CMAKE_DL_LIBS})
endif()

target_link_libraries(${EXEC_NAME} 
            

            model
            graphicmesh
						#freetype
            io
					  #backend
					  glfw3
					  #common
					  ${LIBS}
            ${RT_LIB}
            ${VTK_LIBRARIES}
	    
            ${GMSH_LIBS}
            ${LINK_LIBRARIES}
)


file (GLOB SOURCE_FILES "${CMAKE_SOURCE_DIR}/src/model/*.cpp")

find_package(pybind11 REQUIRED)
pybind11_add_module(py_imgui-vtk 
	${SOURCE_FILES}
	${HEADER_FILES}
	${PYTHON_FILES}
)
