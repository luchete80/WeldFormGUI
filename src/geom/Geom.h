#ifndef _GEOM_H_
#define _GEOM_H_

#include <string>
#include <TopoDS_Shape.hxx> // OpenCascade shape
#include "vtkOCCTGeom.h"


class Geom{
public:
  Geom(std::string fname);
  void readFile(std::string file);  

  std::string m_filename; //
  std::string m_name;
  double scale; //Is scale
  TopoDS_Shape shape;       // Lógica CAD
  vtkOCCTGeom* visual = nullptr;

  void createVisual(); // Crea visual si no existe
  void deleteVisual(); // Borra el actor del visualizador

  vtkActor* getActor() {
    if (!visual) return nullptr;
    return visual->actor;
  }   
  
};

#endif
