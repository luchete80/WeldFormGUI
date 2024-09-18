# WeldFormGUI
WeldForm & WeldFormGPU simple, intuitive, native and light 
Graphical User Interface to generate model and export solver input


![alt text](https://github.com/luchete80/WeldFormGUI/blob/main/image.png)


## Build Instructions

On ubuntu 
sudo apt-get install xorg-dev libglu1-mesa-dev

## Geometry import

You need opencocct-V7_5_0




Integrate FreetypeGL

https://www.youtube.com/watch?v=WFvb7SPcQbg
https://hackage.haskell.org/package/FreeTypeGL-0.0.4

Repolace text rendering ogl

https://learnopengl.com/In-Practice/Text-Rendering


For compiling OCC (importing iges and step files)
https://gitlab.onelab.info/gmsh/gmsh/-/wikis/Gmsh-compilation

WeldFormGUI uses gmsh and "contrib/netgen/libsrc/occ"

On Ubuntu
curl -L -o occt.tgz "http://git.dev.opencascade.org/gitweb/?p=occt.git;a=snapshot;h=refs/tags/V7_5_0;sf=tgz"
tar zxf occt.tgz
cd occt-V7_5_0
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_MODULE_Draw=0 -DBUILD_MODULE_Visualization=0 -DBUILD_MODULE_ApplicationFramework=0 ..

On MSWIN
https://github.com/Open-Cascade-SAS/OCCT/archive/refs/tags/V7_5_0.tar.gz

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
cmake [SOURCE_DIR] -DVTK_INCLUDE_DIR="/usr/local/include/vtk-9.3" -DCMAKE_BUILD_TYPE=RELEASE 

Should build VTK with
cmake .......  -DVTK_MODULE_ENABLE_VTK_IOOCCT=ON
