#ifndef MODEL_WRITER_H
#define MODEL_WRITER_H

#include <string>

class Model;
//class json;

class ModelWriter{
public:
  ModelWriter(Model &Model);
  //ModelWriter(char *fname);
  void writeToFile(std::string fname);
  ~ModelWriter(){};  

protected:
  char  *m_filename;
  Model &m_model;
};

#endif
