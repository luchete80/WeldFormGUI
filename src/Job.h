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
  void UpdateOutput();
  std::string & getPathFile(){return m_path_file;}
  std::string & getLog(){return m_log;  }
protected:
  std::string m_path_file;
  int m_pid;
  std::string m_filename;
  std::string m_log;
  
};






#endif
