#include "Job.h"
#include <cstdlib>


Job::Job(std::string str){
  m_path_file = str;
}

int Job::Run(){
  std::string str = "solvers/WeldForm " + m_path_file;
  int returnCode = system(str.c_str());
  
  return returnCode;
  
}
