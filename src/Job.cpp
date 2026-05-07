#include "Job.h"
#include <cstdlib>

#include <iostream>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <deque>
#include <cstdio>
#include <algorithm>
#include <cctype>
#include <sstream>

using namespace std;
using json = nlohmann::json;
namespace fs = std::filesystem;

namespace {
std::string toLowerCopy(std::string value)
{
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  return value;
}

bool envVarEnabled(const char* name, bool defaultValue = false)
{
  const char* value = std::getenv(name);
  if (value == nullptr) {
    return defaultValue;
  }

  const std::string normalized = toLowerCopy(value);
  return normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on";
}

int envVarInt(const char* name, int defaultValue)
{
  const char* value = std::getenv(name);
  if (value == nullptr) {
    return defaultValue;
  }

  try {
    return std::stoi(value);
  } catch (...) {
    return defaultValue;
  }
}

int countBdfNodes(const fs::path& path)
{
  std::ifstream file(path);
  if (!file.is_open()) {
    return -1;
  }

  int count = 0;
  std::string line;
  while (std::getline(file, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }

    if (line.rfind("GRID", 0) == 0 || line.rfind("GRID*", 0) == 0) {
      ++count;
    }
  }

  return count;
}
}


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

fs::path Job::getPidFilePath() const{
  fs::path input_path(m_path_file);
  std::string stem = input_path.stem().string();
  if (stem.empty())
    stem = "job";
  return getJobDirectory() / (stem + ".pid");
}

int Job::getPid() const{
  if (m_pid > 0)
    return m_pid;

  const fs::path pid_path = getPidFilePath();
  if (!fs::exists(pid_path))
    return -1;

  std::ifstream file(pid_path);
  int pid = -1;
  if (!(file >> pid))
    return -1;

  return pid;
}

bool Job::isImplicit() const{
  std::optional<json> input = loadInputJson();
  if (!input.has_value())
    return false;

  if (!input->contains("Configuration"))
    return false;

  const auto &conf = (*input)["Configuration"];
  if (!conf.contains("solver"))
    return false;

  if (conf["solver"].is_object() && conf["solver"].contains("implicit"))
    return true;

  return false;
}

std::optional<json> Job::loadInputJson() const
{
  std::ifstream file(m_path_file);
  if (!file.is_open()) {
    return std::nullopt;
  }

  try {
    json j;
    file >> j;
    return j;
  } catch (...) {
    return std::nullopt;
  }
}

Job::SolverInputInfo Job::inspectSolverInput() const
{
  SolverInputInfo info;

  std::optional<json> input = loadInputJson();
  if (!input.has_value()) {
    return info;
  }

  if (input->contains("Configuration")) {
    const auto& conf = (*input)["Configuration"];
    info.thermal = conf.value("thermal", false);
    if (conf.contains("solver") &&
        conf["solver"].is_object() &&
        conf["solver"].contains("implicit")) {
      info.implicit = true;
    }
  }

  int totalNodes = 0;
  bool countedAny = false;
  const fs::path inputDir = getJobDirectory();

  auto accumulateNodesFromBlocks = [&](const char* key) {
    if (!input->contains(key) || !(*input)[key].is_array()) {
      return;
    }

    for (const auto& block : (*input)[key]) {
      if (!block.is_object() || !block.contains("fileName") || !block["fileName"].is_string()) {
        continue;
      }

      const fs::path meshPath = inputDir / block["fileName"].get<std::string>();
      const int blockNodes = countBdfNodes(meshPath);
      if (blockNodes >= 0) {
        totalNodes += blockNodes;
        countedAny = true;
      }
    }
  };

  accumulateNodesFromBlocks("DomainBlocks");
  accumulateNodesFromBlocks("RigidBodies");

  if (countedAny) {
    info.nodeCount = totalNodes;
  }

  return info;
}

Job::SolverEdition Job::getRequestedSolverEdition() const
{
  if (m_solver_edition_override != SolverEdition::Auto) {
    return m_solver_edition_override;
  }

  const char* editionEnv = std::getenv("WELDFORM_SOLVER_EDITION");
  if (editionEnv == nullptr) {
    return SolverEdition::Auto;
  }

  const std::string edition = toLowerCopy(editionEnv);
  if (edition == "full") {
    return SolverEdition::Full;
  }
  if (edition == "std" || edition == "student") {
    return SolverEdition::Std;
  }
  return SolverEdition::Auto;
}

std::string Job::getSolverBinaryName(const SolverInputInfo& info) const
{
  const std::string baseName = info.implicit ? "weldform_imp" : "weldform_exp";
  const SolverEdition requestedEdition = getRequestedSolverEdition();
  const fs::path solversDir = fs::current_path() / "solvers";

  const auto existsForCurrentPlatform = [&](const std::string& binaryName) {
#ifdef _WIN32
    return fs::exists(solversDir / (binaryName + ".exe")) || fs::exists(solversDir / binaryName);
#else
    return fs::exists(solversDir / binaryName);
#endif
  };

  if (requestedEdition == SolverEdition::Full) {
    return baseName;
  }

  if (requestedEdition == SolverEdition::Std) {
    return baseName + "_std";
  }

  if (existsForCurrentPlatform(baseName)) {
    return baseName;
  }

  return baseName + "_std";
}

bool Job::validateSolverSelection(const SolverInputInfo& info,
                                  SolverEdition edition,
                                  const std::string& solverName) const
{
  const fs::path solversDir = fs::current_path() / "solvers";
#ifdef _WIN32
  const bool solverExists =
      fs::exists(solversDir / (solverName + ".exe")) || fs::exists(solversDir / solverName);
#else
  const bool solverExists = fs::exists(solversDir / solverName);
#endif

  const auto persistLogMessage = [&](const std::vector<std::string>& lines) {
    const fs::path logPath = getLogFilePath();
    std::error_code ec;
    const fs::path parent = logPath.parent_path();
    if (!parent.empty()) {
      fs::create_directories(parent, ec);
    }

    std::ofstream logFile(logPath, std::ios::out | std::ios::trunc);
    if (!logFile.is_open()) {
      return;
    }

    for (const std::string& line : lines) {
      logFile << line << '\n';
    }
    logFile.flush();
  };

  if (!solverExists) {
    const std::string line1 = "[Job::Run] Solver binary not found: " + solverName;
    const std::string line2 =
        "[Job::Run] Expected location: " + (solversDir / solverName).string();
    std::cout << line1 << std::endl;
    std::cout << line2 << std::endl;
    persistLogMessage({line1, line2});
    return false;
  }

  const bool isStdEdition =
      edition == SolverEdition::Std ||
      (edition == SolverEdition::Auto && solverName.size() >= 4 &&
       solverName.substr(solverName.size() - 4) == "_std");

  if (!isStdEdition) {
    return true;
  }

  const bool allowThermalInStd = envVarEnabled("WELDFORM_STD_ALLOW_THERMAL", false);
  const int stdNodeLimit = envVarInt("WELDFORM_STD_NODE_LIMIT", 500);

  if (info.thermal && !allowThermalInStd) {
    const std::string line1 =
        "[Job::Run] Thermal coupling is disabled for student solver edition.";
    const std::string line2 =
        "[Job::Run] Set WELDFORM_STD_ALLOW_THERMAL=1 to enable it.";
    std::cout << line1 << std::endl;
    std::cout << line2 << std::endl;
    persistLogMessage({line1, line2});
    return false;
  }

  if (info.nodeCount > stdNodeLimit) {
    const std::string line1 =
        "[Job::Run] Student solver node limit exceeded. Nodes=" +
        std::to_string(info.nodeCount) + ", limit=" + std::to_string(stdNodeLimit);
    const std::string line2 =
        "[Job::Run] Set WELDFORM_SOLVER_EDITION=full or increase "
        "WELDFORM_STD_NODE_LIMIT if appropriate.";
    std::cout << line1 << std::endl;
    std::cout << line2 << std::endl;
    persistLogMessage({line1, line2});
    return false;
  }

  return true;
}

int Job::Run(){
  // IF NOT REDIRECTING OUTPUT IS GARBAGE AT PROMPT
  std::string str;
  int returnCode ;
  const SolverInputInfo inputInfo = inspectSolverInput();
  const SolverEdition requestedEdition = getRequestedSolverEdition();
  const std::string solver_name = getSolverBinaryName(inputInfo);
  const std::string log_path = getLogFilePath().string();
  const std::string pid_path = getPidFilePath().string();

  if (!validateSolverSelection(inputInfo, requestedEdition, solver_name)) {
    return -1;
  }

  std::cout << "[Job::Run] Solver selection: "
            << (inputInfo.implicit ? "implicit" : "explicit")
            << ", edition=" << (solver_name.find("_std") != std::string::npos ? "std" : "full")
            << ", thermal=" << (inputInfo.thermal ? "on" : "off");
  if (inputInfo.nodeCount >= 0) {
    std::cout << ", nodes=" << inputInfo.nodeCount;
  }
  std::cout << std::endl;

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

  std::remove(pid_path.c_str());
  m_pid = -1;

  #ifdef _WIN32
  //str = "START /B solvers\\" + solver_name + " \"" + m_path_file + "\" > \"" + log_path + "\"";
  str = "START /B solvers\\" + solver_name + " \"" + m_path_file+ "\"";
  cout << "RUNNING "<<str<<endl;
  #elif linux
  str = "sh -c 'nohup solvers/" + solver_name + " \"" + m_path_file +
        "\" > \"" + log_path + "\" 2>&1 & echo $! > \"" + pid_path + "\"'";
  #endif

  returnCode = system(str.c_str());    

  m_pid = getPid();
  cout << "Process ID "<< m_pid <<endl;
  return returnCode; 
  
}

bool Job::Stop(){
  const int pid = getPid();
  if (pid <= 0) {
    cout << "[Job::Stop] No valid PID available for " << m_path_file << endl;
    return false;
  }

  std::string str;
  int returnCode = -1;

  #ifdef _WIN32
  str = "taskkill /PID " + std::to_string(pid) + " /T /F";
  #elif linux
  str = "kill -TERM " + std::to_string(pid);
  #endif

  returnCode = system(str.c_str());
  if (returnCode != 0) {
    cout << "[Job::Stop] Failed to stop PID " << pid << " for " << m_path_file << endl;
    return false;
  }

  std::remove(getPidFilePath().string().c_str());
  m_pid = -1;
  cout << "[Job::Stop] Stopped PID " << pid << " for " << m_path_file << endl;
  return true;
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
