#include "Job.h"
#include <cstdlib>

#include <iostream>
#include <iostream>
#include <iomanip>
#include <fstream>

using namespace std;


Job::Job(std::string str){
  m_path_file = str;
  cout << "path "<<str<<endl;
}

int Job::Run(){
  std::string str;
  int returnCode ;
  #ifdef _WIN32
  //str = "START /B solvers\\WeldForm " + m_path_file;
  #elif linux
  str = "nohup solvers/WeldForm " + m_path_file + " > my.log 2>&1  &";
  //echo $!
  #endif
  //int returnCode = system(str.c_str());
  //CATCH ID
  #ifdef _WIN32
  //str = "START /B solvers\\WeldForm " + m_path_file;
  #elif linux
  //str = "echo $! > pid.txt";
  //str = "ps -e --sort=start_time | grep \"WeldForm \" ";
  //echo $!
  #endif
  returnCode = system(str.c_str());
  
  cout << "Process ID "<< returnCode<<endl;
  return returnCode; 
  
}
void Job::UpdateOutput(){
  int returnCode;
  string str =  m_path_file.substr(0, m_path_file.find_last_of(".")+1) + "out";
  
  string cpy_str = "cp " + str + " " + "temp.out";
  
  system("rm temp.out");
  returnCode = system(cpy_str.c_str());
  
  ifstream file;
	file.open("temp.out");

	if (file.is_open()) {
		//cout << "[I] Found input file " << fileName << endl;
  string line;
  m_log = "";
  while(getline(file, line))
    m_log += line;
	} else {
		cout << "[E] Input file " << "temp.out" << " could not be found!!" << endl;
	}
  

  
  file.close();  
  
}

void Job::Draw(){
  // if (!ImGui::Begin(title, p_open))
  // {
      // ImGui::End();
      // return;
  // }
 // ImGui::InputDouble("Density ", &m_density_const, 0.00f, 1.0f, "%.4f");  
}


// tasklist /V /FI "WindowTitle eq service1*"
// tasklist /V /FI "WindowTitle eq service2*"
// Kill the processes:

// taskkill /FI "WindowTitle eq service1*" /T /F
// taskkill /FI "WindowTitle eq service2*" /T /F

// This will run my_command saving all output into my.log (in a script, $! represents the PID of the last process executed). The 2 is the file descriptor for standard error (stderr) and 2>&1 tells the shell to route standard error output to the standard output (file descriptor 1). It requires &1 so that the shell knows it's a file descriptor in that context instead of just a file named 1. The 2>&1 is needed to capture any error messages that normally are written to standard error into our my.log file (which is coming from standard output). See I/O Redirection for more details on handling I/O redirection with the shell.

// If the command sends output on a regular basis, you can check the output occasionally with tail my.log, or if you want to follow it "live" you can use tail -f my.log. Finally, if you need to kill the process, you can do it via:

// kill -9 `cat save_pid.txt`
// rm save_pid.txt