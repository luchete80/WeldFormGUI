#ifndef _JOB_H_
#define _JOB_H_

#include <string>
#include <filesystem>
#include <optional>
#include <nlohmann/json.hpp>

#include "Dialog.h"

class Job:
public Dialog{
public: 
  enum class SolverEdition {
    Auto,
    Std,
    Full
  };

  Job(){}
  Job(std::string );
  virtual void Draw();
  int Run();
  bool Stop();
  void UpdateOutput(int max_lines = 100);
  std::string & getPathFile(){return m_path_file;}
  std::string & getLog(){return m_log;  }
  void setPathFile(const std::string &path){m_path_file = path;}
  std::string getDisplayName() const;
  bool isImplicit() const;
  int getPid() const;
  bool isRunning() const;
  std::filesystem::path getResultsFilePath() const;
  double getExpectedSimTime() const;
  double getCurrentResultTime() const;
  float getEstimatedProgress() const;
  void setSolverEditionOverride(SolverEdition edition) { m_solver_edition_override = edition; }
  SolverEdition getSolverEditionOverride() const { return m_solver_edition_override; }
protected:
  struct SolverInputInfo {
    bool implicit = false;
    bool is3D = true;
    bool thermal = false;
    int nodeCount = -1;
  };

  std::filesystem::path getJobDirectory() const;
  std::filesystem::path getLogFilePath() const;
  std::filesystem::path getTempLogPath() const;
  std::filesystem::path getPidFilePath() const;
  std::optional<nlohmann::json> loadInputJson() const;
  SolverInputInfo inspectSolverInput() const;
  SolverEdition getRequestedSolverEdition() const;
  std::string getSolverBinaryName(const SolverInputInfo& info) const;
  bool validateSolverSelection(const SolverInputInfo& info,
                               SolverEdition edition,
                               const std::string& solverName) const;
  std::string m_path_file;
  int m_pid = -1;
  std::string m_filename;
  std::string m_log;
  SolverEdition m_solver_edition_override = SolverEdition::Auto;
  
};






#endif
