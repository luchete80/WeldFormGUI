#ifndef MODEL_READER_H
#define MODEL_READER_H

#include <nlohmann/json.hpp>

using namespace nlohmann; 
class Model;
//class json;

class ModelReader{
public:
  //ModelReader(){}
  ModelReader(Model*);
  ModelReader(const char *);
  Model* getModel(){return m_model;}
  bool readFromFile(const std::string& fname) ;
  
  ~ModelReader(){delete m_model;}  

protected:
  char  *m_filename;
  Model *m_model;
  json  m_json;
};

#endif
