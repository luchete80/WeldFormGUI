#include "Geom.h"
#include <TopoDS_Shape.hxx> // OpenCascade shape

#include <TopoDS_Shape.hxx>

#include <BRepPrimAPI_MakeBox.hxx>

#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>


//Export STEP
#include <STEPControl_Writer.hxx>
#include <STEPControl_StepModelType.hxx>
#include <IFSelect_ReturnStatus.hxx>

//READ FILE
#include <STEPControl_Reader.hxx>
#include <BRepTools.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <algorithm>
#include <cmath>
#include <iostream>

#include <BRepBuilderAPI_Transform.hxx> // Necesario para transformaciones
#include <gp_Trsf.hxx>                 // Necesario para definición de transformaciones
#include <BRepBndLib.hxx>
#include <BRepGProp.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>

#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeRevol.hxx>

#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>

namespace {
constexpr double kPi = 3.14159265358979323846;

gp_Dir makeDirectionOrDefault(double x, double y, double z)
{
    const double norm = std::sqrt(x * x + y * y + z * z);
    if (norm <= 1.0e-12) {
        return gp_Dir(0.0, 0.0, 1.0);
    }

    return gp_Dir(x, y, z);
}
}

void Geom::Move(const double &dx, const double &dy, const double &dz){

    if (!m_shape) {
        std::cerr << "Error: No Shape to move." << std::endl;
        return;
    }

    //
    gp_Trsf tr;
    tr.SetTranslation(gp_Vec(dx, dy, dz));

    // 2
    BRepBuilderAPI_Transform transformer(*m_shape, tr, true); // true = copia
    TopoDS_Shape movedShape = transformer.Shape();

    if (movedShape.IsNull()) {
        std::cerr << "Error: la transformación falló." << std::endl;
        return;
    }
  
    //
    *m_shape = movedShape;

    m_origin.x += dx;
    m_origin.y += dy;
    m_origin.z += dz;
    
}

bool Geom::Scale(const double &factor){
    if (!m_shape) {
        std::cerr << "Error: No Shape to scale." << std::endl;
        return false;
    }

    if (factor <= 0.0) {
        std::cerr << "Error: scale factor must be greater than zero." << std::endl;
        return false;
    }

    Bnd_Box bbox;
    BRepBndLib::Add(*m_shape, bbox);

    double xMin = 0.0, yMin = 0.0, zMin = 0.0;
    double xMax = 0.0, yMax = 0.0, zMax = 0.0;
    bbox.Get(xMin, yMin, zMin, xMax, yMax, zMax);

    gp_Pnt center((xMin + xMax) * 0.5,
                  (yMin + yMax) * 0.5,
                  (zMin + zMax) * 0.5);

    gp_Trsf tr;
    tr.SetScale(center, factor);

    BRepBuilderAPI_Transform transformer(*m_shape, tr, true);
    TopoDS_Shape scaledShape = transformer.Shape();
    if (scaledShape.IsNull()) {
        std::cerr << "Error: scale transformation failed." << std::endl;
        return false;
    }

    *m_shape = scaledShape;

    gp_Pnt originPoint(m_origin.x, m_origin.y, m_origin.z);
    originPoint.Transform(tr);
    m_origin.x = originPoint.X();
    m_origin.y = originPoint.Y();
    m_origin.z = originPoint.Z();
    return true;
}

bool Geom::Rotate(double angleDeg,
                  double axisDirX,
                  double axisDirY,
                  double axisDirZ){
    if (!m_shape) {
        std::cerr << "Error: No Shape to rotate." << std::endl;
        return false;
    }

    const double directionNorm = std::sqrt(axisDirX * axisDirX +
                                           axisDirY * axisDirY +
                                           axisDirZ * axisDirZ);
    if (directionNorm <= 1.0e-12) {
        std::cerr << "Error: rotation axis must be non-zero." << std::endl;
        return false;
    }

    const double angleRad = angleDeg * kPi / 180.0;
    gp_Trsf tr;
    tr.SetRotation(gp_Ax1(gp_Pnt(m_origin.x, m_origin.y, m_origin.z),
                          gp_Dir(axisDirX, axisDirY, axisDirZ)),
                   angleRad);

    BRepBuilderAPI_Transform transformer(*m_shape, tr, true);
    TopoDS_Shape rotatedShape = transformer.Shape();
    if (rotatedShape.IsNull()) {
        std::cerr << "Error: rotation transformation failed." << std::endl;
        return false;
    }

    *m_shape = rotatedShape;
    return true;
}

bool Geom::RotateAroundPoint(double angleDeg,
                             double axisDirX,
                             double axisDirY,
                             double axisDirZ,
                             double pivotX,
                             double pivotY,
                             double pivotZ){
    if (!m_shape) {
        std::cerr << "Error: No Shape to rotate." << std::endl;
        return false;
    }

    const double directionNorm = std::sqrt(axisDirX * axisDirX +
                                           axisDirY * axisDirY +
                                           axisDirZ * axisDirZ);
    if (directionNorm <= 1.0e-12) {
        std::cerr << "Error: rotation axis must be non-zero." << std::endl;
        return false;
    }

    const double angleRad = angleDeg * kPi / 180.0;
    gp_Trsf tr;
    tr.SetRotation(gp_Ax1(gp_Pnt(pivotX, pivotY, pivotZ),
                          gp_Dir(axisDirX, axisDirY, axisDirZ)),
                   angleRad);

    BRepBuilderAPI_Transform transformer(*m_shape, tr, true);
    TopoDS_Shape rotatedShape = transformer.Shape();
    if (rotatedShape.IsNull()) {
        std::cerr << "Error: rotation transformation failed." << std::endl;
        return false;
    }

    *m_shape = rotatedShape;

    gp_Pnt originPoint(m_origin.x, m_origin.y, m_origin.z);
    originPoint.Transform(tr);
    m_origin.x = originPoint.X();
    m_origin.y = originPoint.Y();
    m_origin.z = originPoint.Z();
    return true;
}

bool Geom::getBoundingBoxCenter(double& centerX, double& centerY, double& centerZ) const
{
    if (!m_shape || m_shape->IsNull()) {
        return false;
    }

    Bnd_Box bbox;
    BRepBndLib::Add(*m_shape, bbox);

    double xMin = 0.0, yMin = 0.0, zMin = 0.0;
    double xMax = 0.0, yMax = 0.0, zMax = 0.0;
    bbox.Get(xMin, yMin, zMin, xMax, yMax, zMax);

    centerX = 0.5 * (xMin + xMax);
    centerY = 0.5 * (yMin + yMax);
    centerZ = 0.5 * (zMin + zMax);
    return true;
}

bool Geom::getMassCenter(double& centerX, double& centerY, double& centerZ) const
{
    if (!m_shape || m_shape->IsNull()) {
        return false;
    }

    GProp_GProps props;
    const TopAbs_ShapeEnum shapeType = m_shape->ShapeType();

    if (shapeType == TopAbs_SOLID || shapeType == TopAbs_COMPSOLID) {
        BRepGProp::VolumeProperties(*m_shape, props);
    } else if (shapeType == TopAbs_FACE || shapeType == TopAbs_SHELL) {
        BRepGProp::SurfaceProperties(*m_shape, props);
    } else {
        BRepGProp::LinearProperties(*m_shape, props);
    }

    if (props.Mass() <= 1.0e-12) {
        return false;
    }

    const gp_Pnt center = props.CentreOfMass();
    centerX = center.X();
    centerY = center.Y();
    centerZ = center.Z();
    return true;
}

// Geom::Geom(std::string fname){
  
  // scale = 1.0;
// }

  void Geom::LoadRectangle(double dx, double dy, double ox, double oy, double oz) {
  gp_Pnt p1(ox,oy,oz), p2(ox+dx,oy,oz), p3(ox+dx,oy+dy,oz), p4(ox,oy+dy,oz);

    m_origin.x = ox;
    m_origin.y = oy;
    m_origin.z = oz;
        
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

void Geom::LoadCylinder(double radius,
                        double height,
                        double angleDeg,
                        double axisDirX,
                        double axisDirY,
                        double axisDirZ) {
    const double clampedAngleDeg = std::max(0.0, std::min(angleDeg, 360.0));
    const bool isFullCylinder = std::abs(clampedAngleDeg - 360.0) <= 1.0e-9;
    TopoDS_Shape cyl;
    if (isFullCylinder) {
        cyl = BRepPrimAPI_MakeCylinder(radius, height).Shape();
    } else {
        const double angleRad = clampedAngleDeg * kPi / 180.0;
        cyl = BRepPrimAPI_MakeCylinder(radius, height, angleRad).Shape();
    }

    delete m_shape;
    m_shape = new TopoDS_Shape(cyl);
    m_origin = make_double3(0.0, 0.0, 0.0);
}

void Geom::LoadBox(double dx, double dy, double dz)
{
    TopoDS_Shape box = BRepPrimAPI_MakeBox(dx, dy, dz).Shape();
    delete m_shape;
    m_shape = new TopoDS_Shape(box);
    m_origin = make_double3(0.0, 0.0, 0.0);
}

bool Geom::LoadRevolvedSTEPProfile(const std::string& fname,
                                   double axisOriginX,
                                   double axisOriginY,
                                   double axisOriginZ,
                                   double axisDirectionX,
                                   double axisDirectionY,
                                   double axisDirectionZ,
                                   double angleDeg)
{
    STEPControl_Reader reader;
    IFSelect_ReturnStatus stat = reader.ReadFile(fname.c_str());

    if (stat != IFSelect_RetDone) {
        std::cerr << "Error: no se pudo leer el perfil STEP " << fname << std::endl;
        return false;
    }

    bool ok = false;
    const int nRoots = reader.NbRootsForTransfer();
    for (int i = 1; i <= nRoots; ++i) {
        ok = reader.TransferRoot(i) || ok;
    }

    if (!ok) {
        std::cerr << "Error: no se pudo transferir el perfil STEP." << std::endl;
        return false;
    }

    TopoDS_Shape profileShape = reader.OneShape();
    if (profileShape.IsNull()) {
        std::cerr << "Error: perfil STEP vacío." << std::endl;
        return false;
    }

    return LoadRevolvedShape(profileShape,
                             axisOriginX,
                             axisOriginY,
                             axisOriginZ,
                             axisDirectionX,
                             axisDirectionY,
                             axisDirectionZ,
                             angleDeg);
}

bool Geom::LoadRevolvedShape(const TopoDS_Shape& profileShape,
                             double axisOriginX,
                             double axisOriginY,
                             double axisOriginZ,
                             double axisDirectionX,
                             double axisDirectionY,
                             double axisDirectionZ,
                             double angleDeg)
{
    if (profileShape.IsNull()) {
        std::cerr << "Error: perfil vacío para revolución." << std::endl;
        return false;
    }

    const double directionNorm = std::sqrt(axisDirectionX * axisDirectionX +
                                           axisDirectionY * axisDirectionY +
                                           axisDirectionZ * axisDirectionZ);
    if (directionNorm <= 1.0e-12) {
        std::cerr << "Error: eje de revolución inválido." << std::endl;
        return false;
    }

    const double clampedAngleDeg = std::max(1.0e-6, std::min(angleDeg, 360.0));
    const double angleRad = clampedAngleDeg * kPi / 180.0;
    gp_Ax1 axis(gp_Pnt(axisOriginX, axisOriginY, axisOriginZ),
                gp_Dir(axisDirectionX, axisDirectionY, axisDirectionZ));

    BRepPrimAPI_MakeRevol revol(profileShape, axis, angleRad);
    revol.Build();
    if (!revol.IsDone()) {
        std::cerr << "Error: falló la revolución del perfil." << std::endl;
        return false;
    }

    TopoDS_Shape revolvedShape = revol.Shape();
    if (revolvedShape.IsNull()) {
        std::cerr << "Error: la revolución generó una shape vacía." << std::endl;
        return false;
    }

    delete m_shape;
    m_shape = new TopoDS_Shape(revolvedShape);
    m_origin = make_double3(axisOriginX, axisOriginY, axisOriginZ);
    return true;
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

bool Geom::LoadClosedPolylineFace(const std::vector<double3>& points)
{
    if (points.size() < 3) {
        std::cerr << "Error: closed profile requires at least 3 points." << std::endl;
        return false;
    }

    BRepBuilderAPI_MakePolygon polygon;
    for (std::size_t i = 0; i < points.size(); ++i) {
        const double3& point = points[i];
        polygon.Add(gp_Pnt(point.x, point.y, point.z));
    }
    polygon.Close();
    polygon.Build();
    if (!polygon.IsDone()) {
        std::cerr << "Error: failed to build polygon wire." << std::endl;
        return false;
    }

    BRepBuilderAPI_MakeFace faceBuilder(polygon.Wire());
    faceBuilder.Build();
    if (!faceBuilder.IsDone()) {
        std::cerr << "Error: failed to build face from closed profile." << std::endl;
        return false;
    }

    TopoDS_Shape face = faceBuilder.Shape();
    if (face.IsNull()) {
        std::cerr << "Error: closed profile produced a null face." << std::endl;
        return false;
    }

    delete m_shape;
    m_shape = new TopoDS_Shape(face);
    m_origin = points.front();
    return true;
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
    m_origin = make_double3(0.0, 0.0, 0.0);
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
    m_origin.x = targetOriginX;
    m_origin.y = targetOriginY;
    m_origin.z = targetOriginZ;
    
    return true;
}
