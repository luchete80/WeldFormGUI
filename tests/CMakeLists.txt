include_directories("${CMAKE_SOURCE_DIR}/libs/gmsh/")
include_directories("${CMAKE_SOURCE_DIR}/libs/gmsh/api")
include_directories("${CMAKE_SOURCE_DIR}/libs/gmsh/src/common")
include_directories("${CMAKE_SOURCE_DIR}/libs/gmsh/src/geo")
include_directories("${CMAKE_SOURCE_DIR}/libs/gmsh/src/graphics")
include_directories("${CMAKE_SOURCE_DIR}/libs/gmsh/src/mesh")
include_directories("${CMAKE_SOURCE_DIR}/libs/gmsh/src/numeric")

add_executable(gmsh_test 	
              gmsh_test.cpp
								) 

if (WIN32)
set (LIBS winmm)
endif()

link_directories(${CMAKE_SOURCE_DIR}/libs)

#target_compile_definitions(editor PUBLIC _GLIBCXX_USE_CXX11_ABI=0)

target_link_libraries(gmsh_test 

            shared
            ${LINK_LIBRARIES}

					  ${LIBS}
           
)




