#include "ShapeToPolyData.h"

#include <unordered_map>
#include <vector>
#include <algorithm>
#include <cmath>

// OpenCASCADE includes
#include <TopoDS.hxx>          // Essential for TopoDS::Face()
#include <TopoDS_Face.hxx>
#include <TopoDS_Edge.hxx>
#include <TopExp_Explorer.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <TopLoc_Location.hxx>
#include <Poly_Triangulation.hxx>
#include <Geom_Curve.hxx>
#include <gp_Circ.hxx>
#include <gp_Pnt.hxx>
#include <GeomAbs_CurveType.hxx>
#include <GCPnts_QuasiUniformDeflection.hxx>


// VTK includes
#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkLine.h>
#include <vtkTriangle.h>
#include <vtkPolyData.h>

vtkSmartPointer<vtkPolyData> ShapeToPolyData(const TopoDS_Shape& shape, double deflection)
{
    Bnd_Box bbox;
    BRepBndLib::Add(shape, bbox);
    double xMin = 0.0, yMin = 0.0, zMin = 0.0;
    double xMax = 0.0, yMax = 0.0, zMax = 0.0;
    bbox.Get(xMin, yMin, zMin, xMax, yMax, zMax);
    const double dx = xMax - xMin;
    const double dy = yMax - yMin;
    const double dz = zMax - zMin;
    const double bboxDiagonal = std::sqrt(dx * dx + dy * dy + dz * dz);
    const double baseCurveDeflection =
        bboxDiagonal > 0.0
            ? std::min(deflection > 0.0 ? deflection : bboxDiagonal * 0.002,
                       std::max(bboxDiagonal * 0.002, 1.0e-7))
            : (deflection > 0.0 ? deflection : 0.01);

    // Mesh shape with OCC
    BRepMesh_IncrementalMesh mesher(shape, deflection);
    
    vtkNew<vtkPoints> points;
    vtkNew<vtkCellArray> triangles;
    vtkNew<vtkCellArray> lines;
    std::unordered_map<int, vtkIdType> pointMap;
    
    for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next())
    {
        TopoDS_Face face = TopoDS::Face(exp.Current());
        TopLoc_Location loc;
        Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(face, loc);
        std::unordered_map<int, vtkIdType> pointMap;
        
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

    for (TopExp_Explorer exp(shape, TopAbs_EDGE); exp.More(); exp.Next())
    {
        const TopoDS_Edge edge = TopoDS::Edge(exp.Current());

        Standard_Real first = 0.0;
        Standard_Real last = 0.0;
        Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
        if (curve.IsNull())
        {
            continue;
        }

        BRepAdaptor_Curve adaptor(edge);
        std::vector<gp_Pnt> edgePoints;

        if (adaptor.GetType() == GeomAbs_Line)
        {
            edgePoints.push_back(curve->Value(first));
            edgePoints.push_back(curve->Value(last));
        }
        else
        {
            double sampledDeflection = baseCurveDeflection;
            if (adaptor.GetType() == GeomAbs_Circle) {
                const double radius = adaptor.Circle().Radius();
                if (radius > 0.0) {
                    sampledDeflection = std::min(sampledDeflection,
                                                 std::max(radius * 0.05, 1.0e-8));
                }
            }
            GCPnts_QuasiUniformDeflection discretizer(adaptor, sampledDeflection);

            if (discretizer.IsDone() && discretizer.NbPoints() >= 2)
            {
                for (int i = 1; i <= discretizer.NbPoints(); ++i)
                {
                    edgePoints.push_back(discretizer.Value(i));
                }
            }
            else
            {
                edgePoints.push_back(curve->Value(first));
                edgePoints.push_back(curve->Value(last));
            }
        }

        for (std::size_t i = 1; i < edgePoints.size(); ++i)
        {
            const gp_Pnt& p0 = edgePoints[i - 1];
            const gp_Pnt& p1 = edgePoints[i];

            vtkIdType id0 = points->InsertNextPoint(p0.X(), p0.Y(), p0.Z());
            vtkIdType id1 = points->InsertNextPoint(p1.X(), p1.Y(), p1.Z());

            vtkNew<vtkLine> line;
            line->GetPointIds()->SetId(0, id0);
            line->GetPointIds()->SetId(1, id1);
            lines->InsertNextCell(line);
        }
    }
    
    vtkNew<vtkPolyData> polyData;
    polyData->SetPoints(points);
    polyData->SetPolys(triangles);
    polyData->SetLines(lines);
    
    return polyData;
}
