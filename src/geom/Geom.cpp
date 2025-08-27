#include "Geom.h"
#include <TopoDS_Shape.hxx> // OpenCascade shape

#include <TopoDS_Shape.hxx>

#include <BRepPrimAPI_MakeBox.hxx>

#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>


//Export STEP
#include <STEPControl_Writer.hxx>
#include <STEPControl_StepModelType.hxx>
#include <IFSelect_ReturnStatus.hxx>


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

void Geom::LoadLine(double dx, double dy, double ox, double oy) {
    gp_Pnt p1(ox, oy, 0);
    gp_Pnt p2(ox+dx, oy+dy, 0);

    // Crear edge (línea entre p1 y p2)
    BRepBuilderAPI_MakeEdge edge(p1, p2);

    // Guardar como shape
    m_shape = new TopoDS_Shape(edge.Shape());
}

bool Geom::ExportSTEP() {
    if (!m_shape) {
        std::cerr << "Error: no hay geometría cargada en m_shape." << std::endl;
        return false;
    }

    STEPControl_Writer writer;
    
    // Transferir la forma al writer
    IFSelect_ReturnStatus status = writer.Transfer(*m_shape, STEPControl_AsIs);
    if (status != IFSelect_RetDone) {
        std::cerr << "Error al transferir la geometría al writer." << std::endl;
        return false;
    }

    // Grabar el archivo STEP
    status = writer.Write(m_fileName.c_str());
    if (status != IFSelect_RetDone) {
        std::cerr << "Error al escribir el archivo STEP." << std::endl;
        return false;
    }

    return true;
}
