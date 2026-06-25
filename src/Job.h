#ifndef _JOB_H_
#define _JOB_H_

#include <string>
#include <filesystem>
#include <optional>
#include <nlohmann/json.hpp>
#include <cstdint>

#include "Dialog.h"
#include "model/Step.h"

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
  void setCheckpointEnabled(bool enabled) { m_checkpoint_enabled = enabled; }
  bool getCheckpointEnabled() const { return m_checkpoint_enabled; }
  void setCheckpointInterval(int interval) { m_checkpoint_interval = interval; }
  int getCheckpointInterval() const { return m_checkpoint_interval; }
  void setCheckpointDir(const std::string& dir) { m_checkpoint_dir = dir; }
  const std::string& getCheckpointDir() const { return m_checkpoint_dir; }
  void setCheckpointPrefix(const std::string& prefix) { m_checkpoint_prefix = prefix; }
  const std::string& getCheckpointPrefix() const { return m_checkpoint_prefix; }
  void setRestartFile(const std::string& path) { m_restart_file = path; }
  const std::string& getRestartFile() const { return m_restart_file; }
  bool hasRestartFile() const { return !m_restart_file.empty(); }
  void setResultBaseName(const std::string& name) { m_result_base_name = name; }
  const std::string& getResultBaseName() const { return m_result_base_name; }
  bool hasResultBaseName() const { return !m_result_base_name.empty(); }
  bool supportsImplicit3DRestart() const;
  bool loadRestartSettingsFromInput();
  bool applyRestartSettingsToInput() const;
protected:
  struct SolverInputInfo {
    bool implicit = false;
    bool is3D = true;
    bool thermal = false;
    ImplicitFormulation implicitFormulation = ImplicitFormulation::RigidViscoplastic;
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
  mutable int m_pid = -1;
  std::string m_filename;
  std::string m_log;
  SolverEdition m_solver_edition_override = SolverEdition::Auto;
  bool m_checkpoint_enabled = false;
  int m_checkpoint_interval = 1;
  std::string m_checkpoint_dir = ".";
  std::string m_checkpoint_prefix = "restart_qt";
  std::string m_restart_file;
  std::string m_result_base_name;
  
};






#endif
