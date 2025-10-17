if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
    set(BUILD_SHARED_LIBS TRUE)
endif()

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

include_directories("${CMAKE_SOURCE_DIR}")
include_directories("../../")
  
find_package(OpenCASCADE REQUIRED)
include_directories(${OpenCASCADE_INCLUDE_DIR})

set( GEOM_SRCS         "vtkOCCTGeom.cpp" 
                       "ShapeToPolyData.cpp"
                       "Geom.cpp")

message (STATUS "VTK Version: ${VTK_VERSION}")
message ("VTK LIBS:" ${VTK_LIBRARIES})

include_directories("${VTK_INCLUDE_DIR}")
include_directories(imgui_dir)
include_directories("${CMAKE_SOURCE_DIR}/libs/imgui")
#if (win32)
#SEPARATE FINDING LIBS
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

ADD_LIBRARY(geom SHARED ${GEOM_SRCS})

#TARGET_LINK_LIBRARIES(geom  ${VTK_LIBRARIES} gmsh) #GMSH_LIBS WITH ERROR
#TARGET_LINK_LIBRARIES(graphicmesh_st  model ${VTK_LIBRARIES} gmsh) #GMSH_LIBS WITH ERROR
  
target_link_libraries(geom
  TKernel        # Core kernel, Standard namespace, memory management
  TKMath         # Math functions
  TKGeomBase     # Basic geometry types (gp_Pnt, gp_Trsf...)
  TKGeomAlgo     # Geometry algorithms
  TKBRep         # Boundary representation
  TKPrim         # Primitives (like cylinders)
  TKTopAlgo      # Topological algorithms
  TKShHealing    # Shape healing
  #TKStdL         # Standard library with implementation (needed for Standard::Free and others)
  TKBinL         # Binary streaming
  TKService      # Services used internally
  TKMesh
  # Add others only if your code uses them:
  # TKMesh, TKXCAF, TKSTEP, etc.
  #TKDESTEP #7.8.0
  #TKSTEP  #7.5.0
  )
  
  string(REPLACE "." ";" OCC_VERSION_LIST ${OpenCASCADE_VERSION})
list(GET OCC_VERSION_LIST 0 OCC_MAJOR)
list(GET OCC_VERSION_LIST 1 OCC_MINOR)
list(GET OCC_VERSION_LIST 2 OCC_PATCH)

# # Convert to integer for comparison
# math(EXPR OCC_VERSION_NUM "${OCC_MAJOR} * 10000 + ${OCC_MINOR} * 100 + ${OCC_PATCH}")
# #set(OCC_750_VERSION_NUM 70500)

 if(OCC_VERSION_NUM GREATER 70500)
    message(STATUS "Using TKSTEP (OCC > 7.5.0)")
    target_link_libraries(geom  ${VTK_LIBRARIES} gmsh TKStdL TKDESTEP )
 else()
    message(STATUS "Using TKDSTEP (OCC <= 7.5.0)")
    target_link_libraries(geom  ${VTK_LIBRARIES} gmsh TKSTEP)
 endif()

# Set destination dir
set(LIB_OUTPUT_DIR ${CMAKE_BINARY_DIR}/src)

add_custom_command(TARGET geom POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:geom>
        ${LIB_OUTPUT_DIR}/$<TARGET_FILE_NAME:geom>
    COMMENT "Copying model to bin/"
)
