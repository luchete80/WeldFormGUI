#ifndef _GEOM_H_
#define _GEOM_H_

#include <string>
#include <TopoDS_Shape.hxx> // OpenCascade shape

//// SHOULD INHERIT FROM SOME SHAPEGEOM  
class Geom{
protected:
  TopoDS_Shape m_shape;       // Lógica CAD
  std::string m_fileName;   // el STEP/IGES original
  
public:
  Geom(){}
  Geom(std::string fname){m_fileName = fname;}
  void readFile(std::string file){}  

  //std::string m_filename; //
  //std::string m_name;
  //double scale; //Is scale

  const TopoDS_Shape& getShape() const { return m_shape; }
  //const double & getScale()const{return scale;}
  ~Geom(){}
    
};

#endif
