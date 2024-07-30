#ifndef _JOB_H_
#define _JOB_H_

#include <string>

#include "Dialog.h"

class Job:
public Dialog{
public: 
  Job(){}
  Job(std::string );
  virtual void Draw();
  int Run();
protected:
  std::string m_path_file;
  
  
};






#endif
