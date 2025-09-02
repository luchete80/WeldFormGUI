include_directories("${CMAKE_SOURCE_DIR}/src")
include_directories("${CMAKE_SOURCE_DIR}/src/App")
include_directories("${CMAKE_SOURCE_DIR}/src/physics")
include_directories("${CMAKE_SOURCE_DIR}/src/model")
include_directories("${CMAKE_SOURCE_DIR}/src/graphicmesh")
include_directories("${CMAKE_SOURCE_DIR}/src/results")
include_directories("${CMAKE_SOURCE_DIR}/libs/ImGuiFileDialog-0.6.4")
include_directories("${CMAKE_SOURCE_DIR}/libs/stb_image") #IN ORDER TO READ TEXTURES
include_directories("${CMAKE_SOURCE_DIR}/libs/gl3w/include")

if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
    set(BUILD_SHARED_LIBS TRUE)
endif()

 
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
add_subdirectory("geom")
add_subdirectory("graphicmesh")
add_subdirectory("io")

add_subdirectory("App")

#Read OCC_LIB_BIN from environment variable, fallback to empty string
#if(DEFINED ENV{OCC_LIB_BIN})
#  set(OCC_LIB_BIN $ENV{OCC_LIB_BIN})
#else()
#  set(OCC_LIB_BIN "")
#endif()

# If OCC_LIB_BIN is set, add it to linker search paths
if(OCC_LIB_BIN)
  message(STATUS "Using OCC_LIB_BIN: ${OCC_LIB_BIN}")
  link_directories(${OCC_LIB_BIN})
else()
  message(WARNING "OCC_LIB_BIN not set; linker may fail if libs are not in standard paths")
endif()

# In this CMakeLists.txt file, components are all built separately and linked together
# If you prefer to compile source files + dependencies (OpenGL loaders, imgui, VtkViewer, etc.)
# together, please see CMakeLists-alt.txt
add_executable(${EXEC_NAME} main.cpp 
                            editor.cpp
                            bc_dialog.cpp
                            material_dialog.cpp
                            set_dialog.cpp
                            ${CMAKE_SOURCE_DIR}/libs/ImGuiFileDialog-0.6.4/ImGuiFileDialog.cpp
                            Job.cpp
                            job_dialog.cpp
                            part_dialog.cpp
                            log.cpp
                            global.cpp
                            ${CMAKE_SOURCE_DIR}/src/LSDynaWriter.cpp
                            geom/vtkOCCTGeom.cpp
                            geom/ShapeToPolyData.cpp
                            
                            #graphics/axis.cpp
                            )
add_library (global_sta STATIC global.cpp)

# OpenGL
find_package(OpenGL REQUIRED)
target_link_libraries(${EXEC_NAME} OpenGL::GL)


# Split version string into components
find_package(OpenCASCADE REQUIRED)
message(STATUS "OCC Version: ${OpenCASCADE_VERSION}")

string(REPLACE "." ";" OCC_VERSION_LIST ${OpenCASCADE_VERSION})
list(GET OCC_VERSION_LIST 0 OCC_MAJOR)
list(GET OCC_VERSION_LIST 1 OCC_MINOR)
list(GET OCC_VERSION_LIST 2 OCC_PATCH)

# Convert to integer for comparison
math(EXPR OCC_VERSION_NUM "${OCC_MAJOR} * 10000 + ${OCC_MINOR} * 100 + ${OCC_PATCH}")
message ("OCC_VERSION_NUM: " ${OCC_VERSION_NUM})
# Example threshold for version 7.5.0
set(OCC_750_VERSION_NUM 70500)


#if (GEOMS_PRIM)
target_link_libraries(${EXEC_NAME} 
  TKernel        # Core kernel, Standard namespace, memory management
  TKMath         # Math functions
  TKGeomBase     # Basic geometry types (gp_Pnt, gp_Trsf...)
  TKGeomAlgo     # Geometry algorithms
  TKBRep         # Boundary representation
  TKPrim         # Primitives (like cylinders)
  TKTopAlgo      # Topological algorithms
  TKShHealing    # Shape healing
  TKStdL         # Standard library with implementation (needed for Standard::Free and others)
  TKBinL         # Binary streaming
  TKService      # Services used internally
  TKMesh
  # Add others only if your code uses them:
  # TKMesh, TKXCAF, TKSTEP, etc.
  #TKDESTEP #7.8.0
  #TKSTEP  #7.5.0
  )
  #
#~ #endif()
# if(OCC_VERSION_NUM GREATER OCC_750_VERSION_NUM)
    # message(STATUS "Using TKDESTEP (OCC > 7.5.0)")
    # target_link_libraries(${EXEC_NAME}  TKDESTEP)
# else()
    # message(STATUS "Using TKSTEP (OCC <= 7.5.0)")
    # #target_link_libraries(${EXEC_NAME}  TKSTEP)
# endif()
  
#~ if (APPLE)
	#~ # Ignore macOS OpenGL deprecation warnings
	#~ target_compile_definitions(${EXEC_NAME} PRIVATE GL_SILENCE_DEPRECATION)
#~ endif()


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
  
  CommonSystem #APP
  vtksys
  QUIET
)

include_directories(${OpenCASCADE_INCLUDE_DIR})
message ("OCCT DIR: " ${THIRDPARTY_DIR})

set (GEOMS_PRIM ON) ####PRIMITIVES



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
  add_definitions(-DBUILD_PYTHON)
  
  # Usar la versión moderna de Python
  find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
  
  message("PYTHON VERSION: ${Python3_VERSION}")
  message("PYTHON INCLUDE DIRS: ${Python3_INCLUDE_DIRS}")
  message("PYTHON LIBRARIES: ${Python3_LIBRARIES}")
  
  # Configurar las variables para compatibilidad
  set(PYTHON_INCLUDE_DIRS ${Python3_INCLUDE_DIRS})
  set(PYTHON_LIBRARIES ${Python3_LIBRARIES})
  target_include_directories(${EXEC_NAME} PRIVATE ${Python3_INCLUDE_DIRS})

  # Verificar si Python.h existe
  find_file(PYTHON_H_FILE Python.h PATHS ${PYTHON_INCLUDE_DIRS})
  if(PYTHON_H_FILE)
    message(STATUS "Python.h found at: ${PYTHON_H_FILE}")
  else()
    message(FATAL_ERROR "Python.h not found in any include directory")
  endif()
  message(STATUS "=== DIAGNÓSTICO PYTHON ===")
  message(STATUS "Python3_VERSION: ${Python3_VERSION}")
  message(STATUS "Python3_INCLUDE_DIRS: ${Python3_INCLUDE_DIRS}")
  message(STATUS "Python3_LIBRARIES: ${Python3_LIBRARIES}")
  message(STATUS "Python3_LIBRARY_DIRS: ${Python3_LIBRARY_DIRS}")
  message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
  
  # Mostrar todas las propiedades del target
  get_target_property(LINK_LIBS ${EXEC_NAME} LINK_LIBRARIES)
  message(STATUS "Current LINK_LIBRARIES: ${LINK_LIBS}")

  # Configuración específica para Python 3.13
  if(Python3_VERSION VERSION_GREATER_EQUAL "3.13")
    message(STATUS "Configurando para Python 3.13+")
    # Python 3.13 requiere definiciones especiales
    add_definitions(-DPy_BUILD_CORE_MODULE)
    add_definitions(-DPy_LIMITED_API)
  endif()
  
# CRÍTICO: Force to use python release onevery mode (Error looking for python313t)
  if(WIN32)
    set_target_properties(${EXEC_NAME} PROPERTIES
      LINK_FLAGS "/NODEFAULTLIB:python313t.lib /NODEFAULTLIB:python313t_d.lib"
    )
    
    # También aplicar a configuraciones específicas
    set_property(TARGET ${EXEC_NAME} PROPERTY LINK_FLAGS_DEBUG "/NODEFAULTLIB:python313t.lib /NODEFAULTLIB:python313t_d.lib")
    set_property(TARGET ${EXEC_NAME} PROPERTY LINK_FLAGS_RELEASE "")
    set_property(TARGET ${EXEC_NAME} PROPERTY LINK_FLAGS_RELWITHDEBINFO "/NODEFAULTLIB:python313t.lib /NODEFAULTLIB:python313t_d.lib")
    set_property(TARGET ${EXEC_NAME} PROPERTY LINK_FLAGS_MINSIZEREL "/NODEFAULTLIB:python313t.lib /NODEFAULTLIB:python313t_d.lib")
  endif()
  
  # -- Search for Swig
  if(NOT DEFINED SWIG_DIR)
      if(DEFINED ENV{SWIG_LIB})
          set(SWIG_DIR "$ENV{SWIG_LIB}" CACHE PATH "Path to SWIG library files")
          message(STATUS "Using SWIG_LIB from environment: ${SWIG_DIR}")
      else()
          message(WARNING "SWIG_DIR not set and SWIG_LIB environment variable not found!")
          message(WARNING "Please set either:")
          message(WARNING "1. SWIG_DIR in CMake (e.g., -DSWIG_DIR=/path/to/swig/Lib)")
          message(WARNING "2. SWIG_LIB environment variable")
      endif()
  endif()

  find_package(SWIG REQUIRED)
  include(${SWIG_USE_FILE})

  # Verify SWIG version
  if(SWIG_VERSION VERSION_LESS "4.3.0")
      message(WARNING "Found SWIG ${SWIG_VERSION} - Recommend upgrading to 4.3.0+")
  endif()

  message(STATUS "SWIG Found:")
  message(STATUS "  - Executable: ${SWIG_EXECUTABLE}")
  message(STATUS "  - Version: ${SWIG_VERSION}")
  message(STATUS "  - Library Path: ${SWIG_DIR}")

  add_subdirectory("python")
endif()  
  
add_executable (app_test  test.cpp)
target_link_libraries(app_test 

app 
#model 
#${PYTHON_LIBRARIES}
)  
  
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
            
            app
            model
            graphicmesh_
						#freetype
            io
					  #backend
					  glfw3
					  #common
					  ${LIBS}
            ${RT_LIB}
            ${VTK_LIBRARIES}
	    
            #${GMSH_LIBS}
            gmsh
            ${LINK_LIBRARIES}
            ${PYTHON_LIBRARIES}
)

