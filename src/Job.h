#ifndef _JOB_H_
#define _JOB_H

#include <string>

class Job {
public: 
  Job(){}
  Job(std::string );
  int Run();
protected:
  std::string m_path_file;
  
  
};






#endif
