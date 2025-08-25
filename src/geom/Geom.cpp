#include "Geom.h"
#include <TopoDS_Shape.hxx> // OpenCascade shape

// Geom::Geom(std::string fname){
  
  // scale = 1.0;
// }

  void Geom::LoadRectangle(double dx, double dy) {
  gp_Pnt p1(0,0,0), p2(dx,0,0), p3(dx,dy,0), p4(0,dy,0);

    // Crear edges
    BRepBuilderAPI_MakeEdge e1(p1,p2);
    BRepBuilderAPI_MakeEdge e2(p2,p3);
    BRepBuilderAPI_MakeEdge e3(p3,p4);
    BRepBuilderAPI_MakeEdge e4(p4,p1);

    // Crear wire
    BRepBuilderAPI_MakeWire wire;
    wire.Add(e1.Edge());
    wire.Add(e2.Edge());
    wire.Add(e3.Edge());
    wire.Add(e4.Edge());

    // Crear face a partir del wire
    TopoDS_Face face = BRepBuilderAPI_MakeFace(wire.Wire());
    // Crear face a partir del wire
    m_shape = new TopoDS_Shape (face);
  }
