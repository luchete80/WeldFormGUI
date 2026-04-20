#include "Job.h"
#include <cstdlib>

#include <iostream>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <deque>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;
namespace fs = std::filesystem;


Job::Job(std::string str){
  m_path_file = str;
  cout << "path "<<str<<endl;
}

fs::path Job::getJobDirectory() const{
  fs::path input_path(m_path_file);
  fs::path dir = input_path.parent_path();
  return dir.empty() ? fs::path(".") : dir;
}

fs::path Job::getLogFilePath() const{
  return getJobDirectory() / "log.txt";
}

fs::path Job::getTempLogPath() const{
  fs::path input_path(m_path_file);
  std::string stem = input_path.stem().string();
  if (stem.empty())
    stem = "job";
  return getJobDirectory() / (stem + "_log_snapshot.tmp");
}

bool Job::isImplicit() const{
  ifstream file(m_path_file);
  if (!file.is_open())
    return false;

  json j;
  try {
    file >> j;
  } catch (...) {
    return false;
  }

  if (!j.contains("Configuration"))
    return false;

  const auto &conf = j["Configuration"];
  if (!conf.contains("solver"))
    return false;

  if (conf["solver"].is_object() && conf["solver"].contains("implicit"))
    return true;

  return false;
}

int Job::Run(){
  // IF NOT REDIRECTING OUTPUT IS GARBAGE AT PROMPT
  std::string str;
  int returnCode ;
  const std::string solver_name = isImplicit() ? "weldform_imp" : "weldform_exp";
  const std::string log_path = getLogFilePath().string();

  //int returnCode = system(str.c_str());
  //CATCH ID
  //REMOVE LOG FIRST
  #ifdef _WIN32
  str =  "del \"" + m_path_file.substr(0, m_path_file.find_last_of(".")+1) + "out\"";  
  //str = "START /B solvers\\WeldForm " + m_path_file;
  #elif linux
  //str = "echo $! > pid.txt";
  //str = "ps -e --sort=start_time | grep \"WeldForm \" ";
  //echo $!
  str =  "rm -f \"" + m_path_file.substr(0, m_path_file.find_last_of(".")+1) + "out\"";  
  #endif
  returnCode = system(str.c_str());  

  #ifdef _WIN32
  //str = "START /B solvers\\" + solver_name + " \"" + m_path_file + "\" > \"" + log_path + "\"";
  str = "START /B solvers\\" + solver_name + " \"" + m_path_file+ " \"";
  cout << "RUNNING "<<str<<endl;
  #elif linux
  str = "nohup solvers/" + solver_name + " \"" + m_path_file + "\" &";
  //str = "nohup solvers/" + solver_name + " \"" + m_path_file + "\" > \"" + log_path + "\" 2>&1 &";
  //echo $!
  #endif

  returnCode = system(str.c_str());    
  
  cout << "Process ID "<< returnCode<<endl;
  return returnCode; 
  
}
void Job::UpdateOutput(int max_lines){
  m_log = "";

  const fs::path log_path = getLogFilePath();
  const fs::path temp_path = getTempLogPath();

  if (!fs::exists(log_path)) {
    cout << "[E] Log file could not be found: " << log_path.string() << endl;
    return;
  }

  try {
    fs::copy_file(log_path, temp_path, fs::copy_options::overwrite_existing);
  } catch (const std::exception& e) {
    cout << "[E] Could not snapshot log file " << log_path.string() << ": " << e.what() << endl;
    return;
  }

  ifstream file(temp_path);
  if (!file.is_open()) {
    cout << "[E] Snapshot file could not be opened: " << temp_path.string() << endl;
    return;
  }

  int l = 0;
  string line;
  std::deque<std::string> last_lines;
  while (getline(file, line)) {
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    if (max_lines > 0) {
      last_lines.push_back(line);
      if ((int)last_lines.size() > max_lines)
        last_lines.pop_front();
    } else {
      m_log += line + "\n";
    }
    l++;
  }

  if (max_lines > 0) {
    for (const auto& buffered_line : last_lines)
      m_log += buffered_line + "\n";
  }

  cout << "File size " << l << endl;
  cout << m_log << endl;
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
