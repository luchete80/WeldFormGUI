#include "ShapeToPolyData.h"

// OpenCASCADE includes
#include <TopoDS.hxx>          // Essential for TopoDS::Face()
#include <TopoDS_Face.hxx>
#include <TopExp_Explorer.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <TopLoc_Location.hxx>
#include <Poly_Triangulation.hxx>
#include <gp_Pnt.hxx>


// VTK includes
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkTriangle.h>
#include <vtkPolyData.h>

vtkSmartPointer<vtkPolyData> ShapeToPolyData(const TopoDS_Shape& shape, double deflection)
{
    // Mesh shape with OCC
    BRepMesh_IncrementalMesh mesher(shape, deflection);
    
    vtkNew<vtkPoints> points;
    vtkNew<vtkCellArray> triangles;
    std::unordered_map<int, vtkIdType> pointMap;
    
    for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next())
    {
        TopoDS_Face face = TopoDS::Face(exp.Current());
        TopLoc_Location loc;
        Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(face, loc);
        
        if (tri.IsNull())
            continue;
            
        // Use Triangle() instead of the deprecated Triangles()
        for (int i = 1; i <= tri->NbTriangles(); ++i)
        {
            int n1, n2, n3;
            tri->Triangle(i).Get(n1, n2, n3);
            
            // Add points if not already added
            for (int node : {n1, n2, n3})
            {
                if (pointMap.find(node) == pointMap.end())
                {
                    gp_Pnt p = tri->Node(node).Transformed(loc.Transformation());
                    vtkIdType id = points->InsertNextPoint(p.X(), p.Y(), p.Z());
                    pointMap[node] = id;
                }
            }
            
            // Add triangle
            vtkNew<vtkTriangle> triangle;
            triangle->GetPointIds()->SetId(0, pointMap[n1]);
            triangle->GetPointIds()->SetId(1, pointMap[n2]);
            triangle->GetPointIds()->SetId(2, pointMap[n3]);
            triangles->InsertNextCell(triangle);
        }
    }
    
    vtkNew<vtkPolyData> polyData;
    polyData->SetPoints(points);
    polyData->SetPolys(triangles);
    
    return polyData;
}