We need graphics mesh, model and several things as dll in case of python scripting.


Class | Responsibility
-- | --
App | Knows the model state (meshes, geometries)
Viewer or Scene | Knows how to display the actors
GraphicMesh / vtkOCCTGeom | Handles VTK creation, not rendering
Main() | Just calls viewer.updateFromApp(getApp())
=======
Role | Description
-- | --
🔄 Converts | Converts TopoDS_Shape (CAD geometry from OpenCascade) → vtkPolyData
🎭 Provides | Exposes a vtkActor* that you can add to your VTK scene
📦 Contains | Stores the TopoDS_Shape, polydata, mapper, actor, etc.
🛠️ Methods | Has loaders like LoadSTEP(), LoadBrep(), LoadCylinder()


>>>>>>> 9e127b042eafe91b6a68a710c88e18e9d54cdc34

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


```cpp
/* -------------------------------------------------------------------------- */
/* 2) Convenience: load geometry from file and auto‑create visual+actor       */
Geom* App::loadGeometry(const std::string& file)
{
    Geom* geom = new Geom(file);          // ctor loads the STEP/BREP
    addOrphanGeometry(geom);
    return geom;
}

/* -------------------------------------------------------------------------- */
/* 3) Sync orphan geometries with viewer                                      */
void App::updateGeoms(/*vtkViewer* viewer*/) {
    std::cout << "Updating orphan geometries: " << m_orphangeoms.size() << std::endl;

    for (Geom* g : m_orphangeoms) {
        // Check if already visualized
        if (geomToVisual.find(g) == geomToVisual.end()) {
            //std::cout << "Creating visual for: " << g->m_name << std::endl;

            vtkOCCTGeom* visual = new vtkOCCTGeom();
            visual->SetGeometry(g);         // Use the shape from Geom
            visual->BuildVTKData();              // Build VTK actor
            visual->isRendered = false;
            
            geomToVisual[g] = visual;            // Store in the map

            //~ if (viewer) {
                //~ viewer->addActor(visual->actor); // Add actor to the viewer (optional here)
            //~ }
        }
    }
}
```