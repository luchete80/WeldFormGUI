#ifndef MODEL_WRITER_H
#define MODEL_WRITER_H

class Model;
//class json;

class ModelWriter{
public:
  ModelWriter(Model &Model);
  //ModelWriter(char *fname);
  void writeToFile();
  ~ModelWriter(){};  

protected:
  char  *m_filename;
  Model &m_model;
};

#endif
