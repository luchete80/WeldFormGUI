# WeldFormGUI
WeldForm & WeldFormGPU simple, intuitive, native and light 
Graphical User Interface to generate model and export solver input


![alt text](https://github.com/luchete80/WeldFormGUI/blob/main/image_01.png)
![alt text](https://github.com/luchete80/WeldFormGUI/blob/main/image_02.png

![alt text](https://github.com/luchete80/WeldFormGUI/blob/main/python_GUI.gif)

## Build Instructions

On ubuntu 
sudo apt-get install xorg-dev libglu1-mesa-dev

## Geometry import

You need opencocct-V7_5_0


Windows 

I prefer to install using nmake, not MSVC project files. 
cmake ..\WeldFormGUI -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=RELEASE \

-DPYTHON_INCLUDE_DIRS=C:\Users\54113\AppData\Local\Programs\Python\Python311\include \

-DPYTHON_LIBRARY=C:\Users\54113\AppData\Local\Programs\Python\Python311

Example: 
cmake ..\WeldFormGUI -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=RELEASE -DVTK_INCLUDE_DIR="/usr/local/include/vtk-9.3" \

-DCMAKE_BUILD_TYPE=RELEASE -DVTK_MODULE_ENABLE_VTK_IOOCCT=ON \
-DCMAKE_PREFIX_PATH=D:\Luciano\Numerico\Libs\occt-vc143-64\cmake\
-DPYTHON_INCLUDE_DIRS=C:\Users\54113\AppData\Local\Programs\Python\Python311\include \
-DPYTHON_LIBRARY=C:\Users\54113\AppData\Local\Programs\Python\Python311



Integrate FreetypeGL

https://www.youtube.com/watch?v=WFvb7SPcQbg
https://hackage.haskell.org/package/FreeTypeGL-0.0.4

Repolace text rendering ogl

https://learnopengl.com/In-Practice/Text-Rendering


For compiling OCC (importing iges and step files)
https://gitlab.onelab.info/gmsh/gmsh/-/wikis/Gmsh-compilation

WeldFormGUI uses gmsh and "contrib/netgen/libsrc/occ"

curl -L -o occt.tgz "http://git.dev.opencascade.org/gitweb/?p=occt.git;a=snapshot;h=refs/tags/V7_5_0;sf=tgz"
tar zxf occt.tgz
cd occt-V7_5_0
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_MODULE_Draw=0 -DBUILD_MODULE_Visualization=0 -DBUILD_MODULE_ApplicationFramework=0 ..
# Notes:
# * if you installed dependencies (e.g. Freetype) in non-standard locations, add the option -DCMAKE_PREFIX_PATH=path-of-installed-dependencies
# * if you don't have root access, add -DCMAKE_INSTALL_PREFIX=path-to-install
# * to build static libraries, add -DBUILD_LIBRARY_TYPE=Static
make
sudo make install
# Notes:
# * if you don't have root access, remove "sudo"

This renders VTK to a FBO
https://github.com/trlsmax/imgui-vtk

MUST SPECIFY IMGUI AND GLFW3 DIRS TO IMUI-VTK

PUT VTK BINARYPATH 
BUILD IT AS RELEASE!
cmake [SOURCE_DIR] -DVTK_INCLUDE_DIR="/usr/local/include/vtk-9.3" -DCMAKE_BUILD_TYPE=RELEASE -DVTK_MODULE_ENABLE_VTK_IOOCCT=ON

Should build VTK with
cmake .......  -DVTK_MODULE_ENABLE_VTK_IOOCCT=ON


-- MS Windows Build Instructions

- Download OpenCascade  7.8.0 built binaries

https://github.com/Open-Cascade-SAS/OCCT/releases/tag/V7_8_0


- Important, from compiled and installed OCCT library run "OCCT/env.bat" --BEFORE-- execute cmake.
 


GMSH USED IS 4.13.0 (4.13.1 DOES NOT WORK ON MSWIN)

You can put the following: ´´´
set(DEFAULT OFF CACHE INTERNAL "Default value for enabled-by-default options")

macro(opt OPTION HELP VALUE)
  option(ENABLE_${OPTION} ${HELP} ${VALUE})
  set(OPT_TEXI "${OPT_TEXI}\n@item ENABLE_${OPTION}\n${HELP} (default: ${VALUE})")
endmacro()

set (ENABLE_NETGEN OFF)
set (ENABLE_POST OFF)
set(CMAKE_BUILD_TYPE Release)
#IF (WIN32)
#set (ENABLE_BUILD_DYNAMIC OFF) #OR ENABLE_BUILD_LIB OR ENABLE_BUILD_SHARED
#set (ENABLE_BUILD_LIB ON)
#ELSE()
set (ENABLE_BUILD_DYNAMIC ON) #OR ENABLE_BUILD_LIB OR ENABLE_BUILD_SHARED
set (ENABLE_BUILD_LIB OFF)
#ENDIF()
set (HAVE_FLTK OFF)
set (ENABLE_GRAPHICS OFF)
set (ENABLE_MESH ON)
set (ENABLE_PARSER OFF)
´´´
ATTENTION; IF OPENCASCADE VERSION IS GREATER OR EQUAL THAN 7.8.0,
VTK version 9.3.1 does not have right targets.

https://github.com/Kitware/VTK/blob/master/IO/OCCT/CMakeLists.txt

if (OpenCASCADE_VERSION VERSION_GREATER_EQUAL "7.8.0")
  set(opencascade_req_targets
    TKDESTEP
    TKDEIGES
    TKernel
    TKMath
    TKMesh
    TKBRep
    TKXSBase
    TKLCAF
    TKXCAF)
else()
  set(opencascade_req_targets
    TKSTEP
    TKIGES
    TKMesh
    TKXDESTEP
    TKXDEIGES)
endif() 
