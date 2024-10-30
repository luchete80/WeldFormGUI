#ifndef MODEL_WRITER_H
#define MODEL_WRITER_H

#include <nlohmann/json.hpp>

using namespace nlohmann; 
class Model;
//class json;

class ModelWriter{
public:
  ModelWriter(Model &Model);
  //ModelWriter(char *fname);
  
  ~ModelWriter(){};  

protected:
  char  *m_filename;
  Model *m_model;
  json  m_json;
};

#endif
