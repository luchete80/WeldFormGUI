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
#include <set>
#include <vector>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

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

fs::path executableDirectory()
{
#ifdef _WIN32
  std::vector<char> buffer(MAX_PATH, '\0');
  DWORD length = 0;
  while (true) {
    length = GetModuleFileNameA(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0) {
      return {};
    }
    if (length < buffer.size() - 1) {
      break;
    }
    buffer.resize(buffer.size() * 2, '\0');
  }
  return fs::path(std::string(buffer.data(), length)).parent_path();
#else
  std::vector<char> buffer(1024, '\0');
  while (true) {
    const ssize_t length = ::readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
    if (length < 0) {
      return {};
    }
    if (length < static_cast<ssize_t>(buffer.size() - 1)) {
      return fs::path(std::string(buffer.data(), static_cast<std::size_t>(length))).parent_path();
    }
    buffer.resize(buffer.size() * 2, '\0');
  }
#endif
}

std::vector<fs::path> solverSearchDirectories(const fs::path& jobDirectory)
{
  std::vector<fs::path> directories;
  const fs::path exeDir = executableDirectory();
  const fs::path cwd = fs::current_path();

  if (!exeDir.empty()) {
    directories.push_back(exeDir / "solvers");
    directories.push_back(exeDir.parent_path() / "solvers");
  }

  directories.push_back(cwd / "solvers");
  directories.push_back(cwd.parent_path() / "WeldFormGUI" / "solvers");

  if (!jobDirectory.empty()) {
    directories.push_back(jobDirectory / "solvers");
  }

  std::vector<fs::path> uniqueDirectories;
  std::set<std::string> seen;
  for (const fs::path& directory : directories) {
    if (directory.empty()) {
      continue;
    }

    const fs::path keyPath = directory.lexically_normal();
    const std::string key = keyPath.string();
    if (seen.insert(key).second) {
      uniqueDirectories.push_back(keyPath);
    }
  }

  return uniqueDirectories;
}

std::vector<fs::path> solverCandidatePaths(const std::string& solverName,
                                           const fs::path& jobDirectory)
{
  std::vector<fs::path> candidates;
  for (const fs::path& directory : solverSearchDirectories(jobDirectory)) {
#ifdef _WIN32
    candidates.push_back(directory / (solverName + ".exe"));
#endif
    candidates.push_back(directory / solverName);
  }
  return candidates;
}

fs::path resolveSolverExecutablePath(const std::string& solverName,
                                     const fs::path& jobDirectory)
{
  for (const fs::path& candidate : solverCandidatePaths(solverName, jobDirectory)) {
    if (fs::exists(candidate)) {
      return candidate;
    }
  }
  return {};
}

std::string joinPathsForLog(const std::vector<fs::path>& paths)
{
  if (paths.empty()) {
    return "<none>";
  }

  std::ostringstream stream;
  for (std::size_t i = 0; i < paths.size(); ++i) {
    if (i > 0) {
      stream << "; ";
    }
    stream << paths[i].string();
  }
  return stream.str();
}

#ifdef _WIN32
std::string formatWindowsErrorMessage(DWORD errorCode)
{
  if (errorCode == 0) {
    return "unknown error";
  }

  LPSTR buffer = nullptr;
  const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_FROM_SYSTEM |
                      FORMAT_MESSAGE_IGNORE_INSERTS;
  const DWORD length = FormatMessageA(flags,
                                      nullptr,
                                      errorCode,
                                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                      reinterpret_cast<LPSTR>(&buffer),
                                      0,
                                      nullptr);
  if (length == 0 || buffer == nullptr) {
    return "error code " + std::to_string(errorCode);
  }

  std::string message(buffer, length);
  LocalFree(buffer);
  while (!message.empty() &&
         (message.back() == '\r' || message.back() == '\n' || message.back() == ' ')) {
    message.pop_back();
  }
  return message;
}
#endif
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
    const std::string domType = conf.value("domType", "3D");
    info.is3D = (domType == "3D");
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
  std::string baseName = "weldform_exp";
  if (info.implicit) {
    baseName = info.is3D ? "weldform_imp_3d" : "weldform_imp";
  }
  const SolverEdition requestedEdition = getRequestedSolverEdition();

  const auto existsForCurrentPlatform = [&](const std::string& binaryName) {
    return !resolveSolverExecutablePath(binaryName, getJobDirectory()).empty();
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
  const fs::path solverExecutable = resolveSolverExecutablePath(solverName, getJobDirectory());
  const bool solverExists = !solverExecutable.empty();

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
        "[Job::Run] Searched: " +
        joinPathsForLog(solverCandidatePaths(solverName, getJobDirectory()));
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
  int returnCode = -1;
  const SolverInputInfo inputInfo = inspectSolverInput();
  const SolverEdition requestedEdition = getRequestedSolverEdition();
  const std::string solver_name = getSolverBinaryName(inputInfo);
  const std::string pid_path = getPidFilePath().string();
  const fs::path absolute_input_path = fs::absolute(fs::path(m_path_file));

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

  const fs::path out_path =
      fs::path(m_path_file.substr(0, m_path_file.find_last_of(".") + 1) + "out");
  std::error_code remove_ec;
  fs::remove(out_path, remove_ec);

  std::remove(pid_path.c_str());
  m_pid = -1;

  #ifdef _WIN32
  const fs::path solver_executable = resolveSolverExecutablePath(solver_name, getJobDirectory());
  const fs::path solver_working_dir =
      solver_executable.has_parent_path() ? solver_executable.parent_path() : fs::current_path();
  const fs::path null_path = "NUL";

  SECURITY_ATTRIBUTES sa{};
  sa.nLength = sizeof(sa);
  sa.bInheritHandle = TRUE;

  HANDLE null_handle = CreateFileA(
      null_path.string().c_str(),
      GENERIC_WRITE,
      FILE_SHARE_READ | FILE_SHARE_WRITE,
      &sa,
      OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL,
      nullptr);
  if (null_handle == INVALID_HANDLE_VALUE) {
    cout << "[Job::Run] Could not open NUL redirection handle." << endl;
    return -1;
  }

  STARTUPINFOA si{};
  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESTDHANDLES;
  si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
  si.hStdOutput = null_handle;
  si.hStdError = null_handle;

  PROCESS_INFORMATION pi{};
  std::string command_line =
      "\"" + solver_executable.string() + "\" \"" + absolute_input_path.string() + "\"";
  std::vector<char> command_buffer(command_line.begin(), command_line.end());
  command_buffer.push_back('\0');

  std::string working_dir = solver_working_dir.string();
  cout << "[Job::Run] Launching solver: " << solver_executable.string() << endl;
  cout << "[Job::Run] Working directory: " << working_dir << endl;
  cout << "[Job::Run] Input file: " << absolute_input_path.string() << endl;
  const BOOL created = CreateProcessA(
      nullptr,
      command_buffer.data(),
      nullptr,
      nullptr,
      TRUE,
      CREATE_NO_WINDOW,
      nullptr,
      working_dir.empty() ? nullptr : working_dir.c_str(),
      &si,
      &pi);

  CloseHandle(null_handle);

  if (!created) {
    const DWORD errorCode = GetLastError();
    const std::string line1 =
        "[Job::Run] CreateProcess failed for " + solver_executable.string();
    const std::string line2 =
        "[Job::Run] Windows error " + std::to_string(errorCode) + ": " +
        formatWindowsErrorMessage(errorCode);
    const std::string line3 = "[Job::Run] Working directory: " + working_dir;
    const std::string line4 = "[Job::Run] Command line: " + command_line;
    cout << line1 << endl;
    cout << line2 << endl;
    cout << line3 << endl;
    cout << line4 << endl;
    std::ofstream logFile(getLogFilePath(), std::ios::out | std::ios::trunc);
    if (logFile.is_open()) {
      logFile << line1 << '\n'
              << line2 << '\n'
              << line3 << '\n'
              << line4 << '\n';
    }
    return -1;
  }

  m_pid = static_cast<int>(pi.dwProcessId);
  cout << "[Job::Run] Started PID " << m_pid << " for " << m_path_file << endl;
  std::ofstream pid_file(pid_path, std::ios::out | std::ios::trunc);
  if (pid_file.is_open()) {
    pid_file << m_pid;
  }

  CloseHandle(pi.hThread);
  Sleep(250);
  DWORD exitCode = STILL_ACTIVE;
  if (GetExitCodeProcess(pi.hProcess, &exitCode)) {
    if (exitCode != STILL_ACTIVE) {
      cout << "[Job::Run] Process exited immediately with code " << exitCode << endl;
      std::ofstream logFile(getLogFilePath(), std::ios::out | std::ios::trunc);
      if (logFile.is_open()) {
        logFile << "[Job::Run] Process exited immediately with code "
                << exitCode << '\n';
        logFile << "[Job::Run] Solver: " << solver_executable.string() << '\n';
        logFile << "[Job::Run] Working directory: " << working_dir << '\n';
        logFile << "[Job::Run] Input file: " << absolute_input_path.string() << '\n';
      }
    }
  }
  CloseHandle(pi.hProcess);
  returnCode = 0;
  #elif linux
  std::string str =
      "sh -c 'nohup solvers/" + solver_name + " \"" + m_path_file +
      "\" >/dev/null 2>&1 & echo $! > \"" + pid_path + "\"'";
  returnCode = system(str.c_str());
  #endif

  m_pid = getPid();
  return returnCode; 
  
}

bool Job::Stop(){
  const int pid = getPid();
  if (pid <= 0) {
    cout << "[Job::Stop] No valid PID available for " << m_path_file << endl;
    return false;
  }

  #ifdef _WIN32
  HANDLE processHandle = OpenProcess(PROCESS_TERMINATE |
                                     PROCESS_QUERY_LIMITED_INFORMATION |
                                     SYNCHRONIZE,
                                     FALSE,
                                     static_cast<DWORD>(pid));
  if (processHandle == nullptr) {
    const DWORD errorCode = GetLastError();
    cout << "[Job::Stop] OpenProcess failed for PID " << pid
         << " (" << errorCode << ": " << formatWindowsErrorMessage(errorCode) << ")" << endl;
    return false;
  }

  DWORD exitCode = STILL_ACTIVE;
  if (GetExitCodeProcess(processHandle, &exitCode) && exitCode != STILL_ACTIVE) {
    CloseHandle(processHandle);
    std::remove(getPidFilePath().string().c_str());
    m_pid = -1;
    cout << "[Job::Stop] PID " << pid << " was already stopped for " << m_path_file << endl;
    return true;
  }

  if (!TerminateProcess(processHandle, 1)) {
    const DWORD errorCode = GetLastError();
    CloseHandle(processHandle);
    cout << "[Job::Stop] TerminateProcess failed for PID " << pid
         << " (" << errorCode << ": " << formatWindowsErrorMessage(errorCode) << ")" << endl;
    return false;
  }

  const DWORD waitResult = WaitForSingleObject(processHandle, 2000);
  CloseHandle(processHandle);
  if (waitResult != WAIT_OBJECT_0) {
    cout << "[Job::Stop] Process " << pid
         << " did not exit within timeout after termination request." << endl;
    return false;
  }
  #elif linux
  std::string str = "kill -TERM " + std::to_string(pid);
  const int returnCode = system(str.c_str());
  if (returnCode != 0) {
    cout << "[Job::Stop] Failed to stop PID " << pid << " for " << m_path_file << endl;
    return false;
  }
  #endif

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
    if (getPid() > 0) {
      m_log = "[Job::Run] Solver started. Waiting for log.txt...\n";
    } else {
      m_log = "[Job::Run] No log.txt available yet.\n";
    }
    return;
  }

  try {
    fs::copy_file(log_path, temp_path, fs::copy_options::overwrite_existing);
  } catch (const std::exception& e) {
    return;
  }

  ifstream file(temp_path);
  if (!file.is_open()) {
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
