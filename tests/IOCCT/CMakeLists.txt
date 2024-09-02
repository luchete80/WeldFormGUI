find_package(VTK 
  CONFIG REQUIRED 
  COMPONENTS 
  CommonCore
  CommonColor
  CommonDataModel
  FiltersCore
  FiltersSources
  InteractionStyle
  RenderingCore
  RenderingContextOpenGL2
  RenderingFreeType
  RenderingGL2PSOpenGL2
  RenderingOpenGL2
  
  IOCore
  IOGeometry
  
  IOOCCT
  CommonExecutionModel
  TestingCore
  TestingRendering
  FiltersGeneral
  
  QUIET
)
  
if (NOT VTK_FOUND)
  message(FATAL_ERROR "VTK not found!")
  return()
endif()


add_executable(test_occ TestOCCTReader.cpp)


if (VTK_VERSION VERSION_LESS "9.0.0")
  include(${VTK_USE_FILE})
else()
  # vtk_module_autoinit is needed
  vtk_module_autoinit( TARGETS test_occ
    MODULES ${VTK_LIBRARIES}
  VTK::CommonCore
  VTK::CommonColor
  VTK::CommonDataModel
  VTK::FiltersCore
  VTK::FiltersSources
  VTK::InteractionStyle
  VTK::RenderingCore
  VTK::RenderingContextOpenGL2
  VTK::RenderingFreeType
  VTK::RenderingGL2PSOpenGL2
  VTK::RenderingOpenGL2
  
  VTK::IOCore
  VTK::IOGeometry
  VTK::IOOCCT
  VTK::InteractionStyle
  VTK::TestingCore
  VTK::TestingRendering
  VTK::FiltersGeneral
  )
endif()
message (STATUS "VTK Version: ${VTK_VERSION}")
message ("VTK LIBS:" ${VTK_LIBRARIES})

target_link_libraries(test_occ ${VTK_LIBRARIES} )

