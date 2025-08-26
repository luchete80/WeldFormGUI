#ifndef _GEOM_H_
#define _GEOM_H_

#include <string>

class TopoDS_Shape; //AVOID OCC (for python wrapping)

//// SHOULD INHERIT FROM SOME SHAPEGEOM  
class Geom{
protected:
  TopoDS_Shape *m_shape;       // Lógica CAD
  std::string m_fileName;   // el STEP/IGES original
  
public:
  Geom(){}
  Geom(std::string fname){m_fileName = fname;}
  void readFile(std::string file){}  
  const std::string getName()const{return m_fileName;}

  //std::string m_filename; //
  //std::string m_name;
  //double scale; //Is scale

  void LoadRectangle(double dx, double dy);
  
  const TopoDS_Shape& getShape() const { return *m_shape; }
  
  void genPlane(const double &edgelength);
  //bool ExportSTEP(const std::string& filename);
  bool ExportSTEP();
  //const double & getScale()const{return scale;}
  ~Geom(){}
    
};

#endif
