#ifndef _GEOM_H_
#define _GEOM_H_

#include <string>

class Geom{
public:
  Geom(std::string fname);
  void readFile(std::string file);  

  std::string m_filename; //
  std::string m_name;
  double scale; //Is scale
  
};

#endif
