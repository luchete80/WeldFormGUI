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

//READ FILE
#include <STEPControl_Reader.hxx>
#include <BRepTools.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <iostream>

#include <BRepBuilderAPI_Transform.hxx> // Necesario para transformaciones
#include <gp_Trsf.hxx>                 // Necesario para definición de transformaciones
#include <BRepBndLib.hxx>


// Geom::Geom(std::string fname){
  
  // scale = 1.0;
// }

  void Geom::LoadRectangle(double dx, double dy, double ox, double oy) {
  gp_Pnt p1(0,0,0), p2(dx,0,0), p3(dx,dy,0), p4(0,dy,0);

    m_origin.x = ox;
    m_origin.y = oy;
    
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
    //m_origin.x = ox;
    //m_origin.y = oy;

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

bool Geom::LoadSTEP(const std::string& fname) {
    STEPControl_Reader reader;
    IFSelect_ReturnStatus stat = reader.ReadFile(fname.c_str());

    if (stat != IFSelect_RetDone) {
        std::cerr << "Error: no se pudo leer el archivo STEP " << fname << std::endl;
        return false;
    }

    // Cargar la raíz (puede haber varias, pero tomamos la primera)
    bool failsonly = false;
    reader.PrintCheckLoad(failsonly, IFSelect_ItemsByEntity);

    // Transferir la geometría a OpenCascade
    int nRoots = reader.NbRootsForTransfer();
    bool ok = false;
    for (int i = 1; i <= nRoots; i++) {
        ok = reader.TransferRoot(i);
    }

    if (!ok) {
        std::cerr << "Error: no se pudo transferir la geometría desde STEP." << std::endl;
        return false;
    }

    // Obtenemos la shape transferida
    TopoDS_Shape shape = reader.OneShape();
    if (shape.IsNull()) {
        std::cerr << "Error: shape vacía al leer STEP." << std::endl;
        return false;
    }

    // Guardamos en el miembro
    m_shape = new TopoDS_Shape(shape);
    m_fileName = fname;
    return true;
}

bool Geom::LoadSTEP(const std::string& fname, double targetOriginX, double targetOriginY, double targetOriginZ) {
    STEPControl_Reader reader;
    IFSelect_ReturnStatus stat = reader.ReadFile(fname.c_str());

    if (stat != IFSelect_RetDone) {
        std::cerr << "Error: no se pudo leer el archivo STEP " << fname << std::endl;
        return false;
    }

    // Cargar la raíz (puede haber varias, pero tomamos la primera)
    bool failsonly = false;
    reader.PrintCheckLoad(failsonly, IFSelect_ItemsByEntity);

    // Transferir la geometría a OpenCascade
    int nRoots = reader.NbRootsForTransfer();
    bool ok = false;
    for (int i = 1; i <= nRoots; i++) {
        ok = reader.TransferRoot(i);
    }

    if (!ok) {
        std::cerr << "Error: no se pudo transferir la geometría desde STEP." << std::endl;
        return false;
    }

    // Obtenemos la shape transferida
    TopoDS_Shape shape = reader.OneShape();
    if (shape.IsNull()) {
        std::cerr << "Error: shape vacía al leer STEP." << std::endl;
        return false;
    }

    // Calcular el bounding box de la shape para encontrar su centro actual
    Bnd_Box bbox;
    BRepBndLib::Add(shape, bbox);
    
    double xMin, yMin, zMin, xMax, yMax, zMax;
    bbox.Get(xMin, yMin, zMin, xMax, yMax, zMax);
    
    // Calcular el centro actual de la pieza
    double currentCenterX = (xMin + xMax) / 2.0;
    double currentCenterY = (yMin + yMax) / 2.0;
    double currentCenterZ = (zMin + zMax) / 2.0;
    
    // Crear transformación para mover la pieza al origen deseado
    gp_Trsf transformation;
    transformation.SetTranslation(
        gp_Vec(-currentCenterX + targetOriginX, 
               -currentCenterY + targetOriginY, 
               -currentCenterZ + targetOriginZ)
    );
    
    // Aplicar la transformación a la shape
    BRepBuilderAPI_Transform transformBuilder(shape, transformation, true);
    TopoDS_Shape transformedShape = transformBuilder.Shape();
    
    // Verificar que la transformación fue exitosa
    if (transformedShape.IsNull()) {
        std::cerr << "Error: la transformación falló." << std::endl;
        return false;
    }

    // Guardamos en el miembro
    m_shape = new TopoDS_Shape(transformedShape);
    m_fileName = fname;
    
    // Actualizar el origen de la geometría
    m_origin.x = targetOriginX;
    m_origin.y = targetOriginY;
    // Si tienes coordenada z en m_origin, también actualízala
    
    return true;
}
