#ifndef _GEOM_H_
#define _GEOM_H_

#include <string>
#include <TopoDS_Shape.hxx>

#include <BRepPrimAPI_MakeBox.hxx>

#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>

//// SHOULD INHERIT FROM SOME SHAPEGEOM  
class Geom{
protected:
  TopoDS_Shape *m_shape;       // Lógica CAD
  std::string m_fileName;   // el STEP/IGES original
  
public:
  Geom(){}
  Geom(std::string fname){m_fileName = fname;}
  void readFile(std::string file){}  

  //std::string m_filename; //
  //std::string m_name;
  //double scale; //Is scale

  void LoadRectangle(double dx, double dy);
  
  const TopoDS_Shape& getShape() const { return *m_shape; }
  
  void genPlane(const double &edgelength);
  
  //const double & getScale()const{return scale;}
  ~Geom(){}
    
};

#endif
