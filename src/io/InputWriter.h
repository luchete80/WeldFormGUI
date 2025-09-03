#ifndef INPUT_WRITER_H
#define INPUT_WRITER_H

#include <nlohmann/json.hpp>

using namespace nlohmann; 
class Model;
//class json;

class InputWriter{
public:
  //InputWriter(){}
  InputWriter(Model*);
  InputWriter(const char *);
  Model* getModel(){return m_model;}
  bool readFromFile(const std::string& fname) ;

  void writeToFile(std::string fname);
  
  ~InputWriter(){}  

protected:
  char  *m_filename;
  Model *m_model;
  json  m_json;
};

#endif
