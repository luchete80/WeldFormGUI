#include "Job.h"
#include <cstdlib>


Job::Job(std::string str){
  m_path_file = str;
}

int Job::Run(){
  std::string str = "solvers\\WeldForm " + m_path_file;
  int returnCode = system(str.c_str());
  
  return returnCode;
  
}

void Job::Draw(){
  // if (!ImGui::Begin(title, p_open))
  // {
      // ImGui::End();
      // return;
  // }
 // ImGui::InputDouble("Density ", &m_density_const, 0.00f, 1.0f, "%.4f");  
}