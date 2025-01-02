#ifndef MODEL_WRITER_H
#define MODEL_WRITER_H

#include <nlohmann/json.hpp>

using namespace nlohmann; 
class Model;
//class json;

class ModelReader{
public:
  ModelReader(const char *);
  Model* getModel(){return m_model;}
  
  ~ModelReader(){delete m_model;}  

protected:
  char  *m_filename;
  Model *m_model;
  json  m_json;
};

#endif
