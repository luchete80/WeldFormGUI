#ifndef _GEOM_H_
#define _GEOM_H_

#include <string>
#include <TopoDS_Shape.hxx> // OpenCascade shape


class Geom{
protected:
  TopoDS_Shape m_shape;       // Lógica CAD
public:
  Geom(){}
  Geom(std::string fname){}
  void readFile(std::string file){}  

  //std::string m_filename; //
  //std::string m_name;
  //double scale; //Is scale

  const TopoDS_Shape& getShape() const { return m_shape; }
  //const double & getScale()const{return scale;}
  ~Geom(){}
    
};

#endif
