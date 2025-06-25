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
  // IF NOT REDIRECTING OUTPUT IS GARBAGE AT PROMPT
  std::string str;
  int returnCode ;

  //int returnCode = system(str.c_str());
  //CATCH ID
  //REMOVE LOG FIRST
  #ifdef _WIN32
  str =  "del" + m_path_file.substr(0, m_path_file.find_last_of(".")+1) + "out";  
  //str = "START /B solvers\\WeldForm " + m_path_file;
  #elif linux
  //str = "echo $! > pid.txt";
  //str = "ps -e --sort=start_time | grep \"WeldForm \" ";
  //echo $!
  str =  "rm" + m_path_file.substr(0, m_path_file.find_last_of(".")+1) + "out";  
  #endif
  returnCode = system(str.c_str());  

  #ifdef _WIN32
  str = "START /B solvers\\WeldFormFEM " + m_path_file + "> log.txt" ;
  #elif linux
  str = "nohup solvers/WeldFormFEM " + m_path_file + " > log.txt 2>&1  &";
  //echo $!
  #endif

  returnCode = system(str.c_str());    
  
  cout << "Process ID "<< returnCode<<endl;
  return returnCode; 
  
}
void Job::UpdateOutput(){
  int returnCode;
  string str;
  
  string cpy_str;
  //cout << "Update.."<<endl;
  #ifdef _WIN32
  system("del temp.out");
  cpy_str = "copy log.txt temp.out";
  #elif linux
  cout << "deleting "<<endl;
  system("rm temp.out");  
  cpy_str = "cp log.txt temp.out";
  //echo $!
  #endif
  
  
  
  
  returnCode = system(cpy_str.c_str());
  
  ifstream file;
	file.open("temp.out");

  int l = 0;
	if (file.is_open()) {
		//cout << "[I] Found input file " << fileName << endl;
  string line;
  m_log = "";

  while(getline(file, line))
    m_log += line + "\n";
    cout << line.c_str()<<endl;
    l++;
	} else {
		cout << "[E] Input file " << "temp.out" << " could not be found!!" << endl;
	}
  cout << "File size "<<l<<endl;
  cout << m_log<<endl;
  
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
