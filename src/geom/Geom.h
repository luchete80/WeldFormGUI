#ifndef _GEOM_H_
#define _GEOM_H_

#include <string>
#include "double3.h"
class TopoDS_Shape; //AVOID OCC (for python wrapping)

class double3;

//// SHOULD INHERIT FROM SOME SHAPEGEOM  
class Geom{
protected:
  TopoDS_Shape *m_shape;       // Lógica CAD
  std::string m_fileName;   // el STEP/IGES original
  double3 m_origin;
  
  
public:
  Geom(){}
  Geom(std::string fname){
    m_fileName = fname;
    LoadSTEP(fname);}
  void readFile(std::string file){}  
  const std::string getName()const{return m_fileName;}

  //std::string m_filename; //
  //std::string m_name;
  //double scale; //Is scale

  void LoadRectangle(double dx, double dy, double ox = 0.0, double oy = 0.0, double oz = 0.0);
  void LoadLine(double dx, double dy, double ox = 0.0, double oy = 0.0);
  bool LoadSTEP(const std::string& fname);
  bool LoadSTEP(const std::string& fname, double targetOriginX, double targetOriginY, double targetOriginZ) ;
  
  void LoadCylinder(double radius, double height);
  
  const double3 & getOrigin()const{return m_origin;}
  
  const TopoDS_Shape& getShape() const { return *m_shape; }
  
  void genPlane(const double &edgelength);
  //bool ExportSTEP(const std::string& filename);
  bool ExportSTEP();
  //const double & getScale()const{return scale;}
  ~Geom(){}
    
};

#endif
