export LIB_DIR=$HOME/Numerico/Libs


cmake -DCMAKE_PREFIX_PATH=$HOME/OCC750-install ../WeldFormGUI -DOpenCASCADE_DIR=$HOME/OCC780-install/lib/cmake/opencascade -DVTK_INCLUDE_DIR=$LIB_DIR/vtk-9.3.1

