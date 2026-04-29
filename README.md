# WeldFormGUI
FEM Graphical User Interaface for large deformation workflows with integrated student explicit and implicit solvers.


![alt text](https://github.com/luchete80/WeldFormGUI/blob/master/ss_1.png)
![alt text](https://github.com/luchete80/WeldFormGUI/blob/master/ss_2.png)

![alt text](https://github.com/luchete80/WeldFormGUI/blob/master/image_01.png)
![alt text](https://github.com/luchete80/WeldFormGUI/blob/master/image_02.png)

![alt text](https://github.com/luchete80/WeldFormGUI/blob/master/python_GUI.gif)

## Build Instructions

On ubuntu 
sudo apt-get install xorg-dev libglu1-mesa-dev

###AFTER BUILD
Run scripts/copiar.sh or copy.bat
Copy all libraries to src

## Geometry import

You need opencocct-V7_5_0

## Meshing workflow

The meshing path depends on the model analysis type and the part type.

- 2D deformable parts:
  the GUI exports the current geometry to STEP and runs the external remesher:
  `mesh-adapt <part.step> <element_size>`
  The remesher is expected to generate a Nastran BDF mesh, which is then copied to the project part mesh file and loaded back into the internal `Mesh` object.

- 2D rigid parts:
  the GUI exports the current geometry to STEP and uses Gmsh only to mesh the boundary curves (`1D` contour mesh). This is intended for rigid contour representations in 2D analyses.

- 3D parts:
  the GUI exports the current geometry to STEP and uses Gmsh to generate the mesh directly.

Current decision logic:

- `PlaneStress2D`, `PlaneStrain2D`, `Axisymmetric2D` + deformable part -> external `mesh-adapt`
- `PlaneStress2D`, `PlaneStrain2D`, `Axisymmetric2D` + rigid part -> Gmsh `1D` contour mesh
- `Solid3D` -> Gmsh mesh generation

### Saved mesh formats

When saving the model, mesh files are stored according to the mesh type actually used:

- 2D deformable meshes are stored as `.bdf`
- 2D rigid contour meshes are also stored as `.bdf`
- 3D Gmsh meshes are stored as `.msh`

The model JSON stores `Configuration.analysisType` and the mesh source path for each part. Relative paths are resolved from the JSON file location when loading the model or loading results.

## Install
See install instructions [here](Install.md)

RESOURCES
----------------------------------
Button images should be moved to the binarie lib in order not to crash
