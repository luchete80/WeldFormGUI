We need graphics mesh, model and several things as dll in case of python scripting.




Class | Responsibility
-- | --
App | Knows the model state (meshes, geometries)
Viewer or Scene | Knows how to display the actors
GraphicMesh / vtkOCCTGeom | Handles VTK creation, not rendering
Main() | Just calls viewer.updateFromApp(getApp())

       +----------------+       has       +-------------------+
       |  vtkOCCTGeom   | <---------------|  GeomCAD (data)   |
       +----------------+                 +-------------------+
            │                                     ▲
            ▼                                     │
 [vtkActor*, mapper, etc]         [TopoDS_Shape, name, etc]
 
 
 
Class | Purpose
-- | --
GraphicMesh | Visual representation of a mesh (SPH, FEM, etc.) from your model. It wraps the mesh and knows how to convert it to vtkPolyData.
vtkOCCTGeom | Visual representation of a CAD geometry (STEP, BREP, primitives like cylinder). It is not tied to mesh.

It have seense to have both


