#ifndef _GEOM_H_
#define _GEOM_H_

#include <string>
#include <vector>
#include "double3.h"
class TopoDS_Shape; //AVOID OCC (for python wrapping)

//// SHOULD INHERIT FROM SOME SHAPEGEOM  
class Geom{
protected:
  TopoDS_Shape *m_shape;       // Lógica CAD
  std::string m_fileName;   // el STEP/IGES original
  double3 m_origin;
  bool m_preferHexaTransfinite = false;
  
  
public:
  Geom() : m_shape(nullptr), m_origin(make_double3(0.0, 0.0, 0.0)) {}
  Geom(std::string fname){
    m_shape = nullptr;
    m_origin = make_double3(0.0, 0.0, 0.0);
    m_fileName = fname;
    LoadSTEP(fname);}
  void readFile(std::string file){}  
  const std::string getName()const{return m_fileName;}
  
  void setFileName(std::string fname){m_fileName = fname;}

  //std::string m_filename; //
  //std::string m_name;
  //double scale; //Is scale

  void LoadRectangle(double dx, double dy, double ox = 0.0, double oy = 0.0, double oz = 0.0);
  void LoadLine(double dx, double dy, double ox = 0.0, double oy = 0.0);
  bool LoadClosedPolylineFace(const std::vector<double3>& points);
  bool LoadSTEP(const std::string& fname);
  bool LoadSTEP(const std::string& fname, double targetOriginX, double targetOriginY, double targetOriginZ) ;
  bool SetShape(const TopoDS_Shape& shape);
  
  void Move(const double &x, const double &y, const double &z);
  bool Scale(const double &factor);
  bool Rotate(double angleDeg,
              double axisDirX,
              double axisDirY,
              double axisDirZ);
  bool RotateAroundPoint(double angleDeg,
                         double axisDirX,
                         double axisDirY,
                         double axisDirZ,
                         double pivotX,
                         double pivotY,
                         double pivotZ);
  
  void LoadBox(double dx, double dy, double dz);
  void LoadCylinder(double radius,
                    double height,
                    double angleDeg = 360.0,
                    double axisDirX = 0.0,
                    double axisDirY = 0.0,
                    double axisDirZ = 1.0);
  bool LoadRevolvedShape(const TopoDS_Shape& profileShape,
                         double axisOriginX,
                         double axisOriginY,
                         double axisOriginZ,
                         double axisDirectionX,
                         double axisDirectionY,
                         double axisDirectionZ,
                         double angleDeg = 360.0);
  bool LoadRevolvedSTEPProfile(const std::string& fname,
                               double axisOriginX,
                               double axisOriginY,
                               double axisOriginZ,
                               double axisDirectionX,
                               double axisDirectionY,
                               double axisDirectionZ,
                               double angleDeg = 360.0);
  bool LoadExtrudedShape(const TopoDS_Shape& profileShape,
                         double directionX,
                         double directionY,
                         double directionZ,
                         double distance);
  bool LoadExtrudedSTEPProfile(const std::string& fname,
                               double directionX,
                               double directionY,
                               double directionZ,
                               double distance);
  
  const double3 & getOrigin()const{return m_origin;}
  void setOrigin(const double3 &origin){m_origin = origin;}
  void setOrigin(double x, double y, double z){m_origin = make_double3(x, y, z);}
  bool getMassCenter(double& centerX, double& centerY, double& centerZ) const;
  bool getBoundingBoxCenter(double& centerX, double& centerY, double& centerZ) const;
  bool SplitByPlane(double originX,
                    double originY,
                    double originZ,
                    double normalX,
                    double normalY,
                    double normalZ,
                    TopoDS_Shape& positiveShape,
                    TopoDS_Shape& negativeShape) const;
  
  const TopoDS_Shape& getShape() const { return *m_shape; }
  bool prefersHexaTransfinite() const { return m_preferHexaTransfinite; }
  void setPreferHexaTransfinite(bool prefer) { m_preferHexaTransfinite = prefer; }
  
  void genPlane(const double &edgelength);
  //bool ExportSTEP(const std::string& filename);
  bool ExportSTEP();
  //const double & getScale()const{return scale;}
  ~Geom(){}
    
};

#endif
