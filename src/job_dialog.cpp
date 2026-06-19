
#include "job_dialog.h"
#include "io/InputWriter.h"
#include "App.h"
#include <iostream>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "ImGuiFileDialog.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <cstring>

using namespace std;
using json = nlohmann::json;
namespace fs = std::filesystem;
namespace {
std::string modelStem(Model &model) {
  std::string model_name = fs::path(model.getName()).stem().string();
  if (model_name.empty())
    model_name = fs::path(model.getName()).filename().string();
  if (model_name.empty())
    model_name = "model";
  return model_name;
}

fs::path modelOutputPath(Model &model, const std::string &filename) {
  return fs::path(model.getBaseDir()) / filename;
}

bool loadRestartConfigurationFromInput(const std::string& filename,
                                       bool& checkpointEnabled,
                                       int& checkpointInterval,
                                       std::string& checkpointDir,
                                       std::string& checkpointPrefix,
                                       std::string& restartFile,
                                       bool& supportsRestart)
{
  checkpointEnabled = false;
  checkpointInterval = 1;
  checkpointDir = "checkpoints";
  checkpointPrefix = "restart_qt";
  restartFile.clear();
  supportsRestart = false;

  if (filename.empty()) {
    return false;
  }

  std::ifstream input(filename);
  if (!input.is_open()) {
    return false;
  }

  try {
    json data;
    input >> data;
    if (!data.contains("Configuration") || !data["Configuration"].is_object()) {
      return false;
    }

    const json& configuration = data["Configuration"];
    if (configuration.value("domType", std::string("3D")) != "3D") {
      return false;
    }

    if (!configuration.contains("solver") || !configuration["solver"].is_object()) {
      return false;
    }

    const json& solver = configuration["solver"];
    if (!solver.contains("implicit") || !solver["implicit"].is_object()) {
      return false;
    }

    supportsRestart = true;
    const json& implicit = solver["implicit"];
    checkpointEnabled = implicit.value("checkpointEnabled", false);
    checkpointInterval = std::max(1, implicit.value("checkpointInterval", 1));
    checkpointDir = implicit.value("checkpointDir", std::string("checkpoints"));
    checkpointPrefix = implicit.value("checkpointPrefix", std::string("restart_qt"));
    restartFile = implicit.value("restartFile", std::string());
    return true;
  } catch (...) {
    return false;
  }
}
}

void JobDialog::resetRestartOptions()
{
  m_checkpoint_enabled = false;
  m_checkpoint_interval = 1;
  m_checkpoint_dir = "checkpoints";
  m_checkpoint_prefix = "restart_qt";
  m_restart_file.clear();
  std::snprintf(m_checkpoint_dir_buffer.data(), m_checkpoint_dir_buffer.size(), "%s", m_checkpoint_dir.c_str());
  std::snprintf(m_checkpoint_prefix_buffer.data(), m_checkpoint_prefix_buffer.size(), "%s", m_checkpoint_prefix.c_str());
  m_restart_file_buffer[0] = '\0';
  m_show_restart_files = false;
}

void JobDialog::loadRestartOptionsFromJob(const Job* job)
{
  if (job == nullptr) {
    resetRestartOptions();
    return;
  }

  m_checkpoint_enabled = job->getCheckpointEnabled();
  m_checkpoint_interval = std::max(1, job->getCheckpointInterval());
  m_checkpoint_dir = job->getCheckpointDir().empty() ? "checkpoints" : job->getCheckpointDir();
  m_checkpoint_prefix = job->getCheckpointPrefix().empty() ? "restart_qt" : job->getCheckpointPrefix();
  m_restart_file = job->getRestartFile();
  std::snprintf(m_checkpoint_dir_buffer.data(), m_checkpoint_dir_buffer.size(), "%s", m_checkpoint_dir.c_str());
  std::snprintf(m_checkpoint_prefix_buffer.data(), m_checkpoint_prefix_buffer.size(), "%s", m_checkpoint_prefix.c_str());
  std::snprintf(m_restart_file_buffer.data(), m_restart_file_buffer.size(), "%s", m_restart_file.c_str());
}

void JobDialog::loadRestartOptionsFromInputFile()
{
  bool supportsRestart = false;
  if (!loadRestartConfigurationFromInput(m_filename,
                                         m_checkpoint_enabled,
                                         m_checkpoint_interval,
                                         m_checkpoint_dir,
                                         m_checkpoint_prefix,
                                         m_restart_file,
                                         supportsRestart)) {
    resetRestartOptions();
    return;
  }

  std::snprintf(m_checkpoint_dir_buffer.data(), m_checkpoint_dir_buffer.size(), "%s", m_checkpoint_dir.c_str());
  std::snprintf(m_checkpoint_prefix_buffer.data(), m_checkpoint_prefix_buffer.size(), "%s", m_checkpoint_prefix.c_str());
  std::snprintf(m_restart_file_buffer.data(), m_restart_file_buffer.size(), "%s", m_restart_file.c_str());
}

bool JobDialog::inputSupportsImplicit3DRestart() const
{
  bool checkpointEnabled = false;
  int checkpointInterval = 1;
  std::string checkpointDir;
  std::string checkpointPrefix;
  std::string restartFile;
  bool supportsRestart = false;
  loadRestartConfigurationFromInput(m_filename,
                                    checkpointEnabled,
                                    checkpointInterval,
                                    checkpointDir,
                                    checkpointPrefix,
                                    restartFile,
                                    supportsRestart);
  return supportsRestart;
}

void  JobDialog::Draw(){

  ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
  create_entity = false;
  //create_job = false; 
  const char *title = m_edit_mode ? "Edit Job" : "Create Job";
  if (!ImGui::Begin(title, nullptr))
  {
      cout << "error drawing" <<endl;
      ImGui::End();
      return;
  }
  //cout << "drawing "<<endl;
  //Vec3_t size;
  ImGui::Text("Path: %s", m_filename.empty() ? "<empty>" : m_filename.c_str());

  const char* solverEditionLabels[] = {"Auto", "Student", "Full"};
  ImGui::Combo("Solver edition", &m_solver_edition, solverEditionLabels, IM_ARRAYSIZE(solverEditionLabels));
  ImGui::TextDisabled("Auto uses environment/default binary selection");

  if (!m_edit_mode && ImGui::Button("From Model")){
    Model &model = getApp().getActiveModel();
    if (model.getHasName()) {
      fs::path input_path = modelOutputPath(model, modelStem(model) + ".wfinput");
      m_filename = input_path.string();
      InputWriter writer(&model);
      writer.writeToFile(input_path.string());
      Job tempJob(m_filename);
      tempJob.setCheckpointEnabled(m_checkpoint_enabled);
      tempJob.setCheckpointInterval(m_checkpoint_interval);
      tempJob.setCheckpointDir(m_checkpoint_dir);
      tempJob.setCheckpointPrefix(m_checkpoint_prefix);
      tempJob.setRestartFile(m_restart_file);
      tempJob.applyRestartSettingsToInput();
      create_entity = true;
      m_edit_mode = false;
      m_job = nullptr;
      m_show = false;
      cout << "Created input from model: " << m_filename << endl;
    } else {
      cout << "File has not name." << endl;
    }
  }

  if (!m_edit_mode)
    ImGui::SameLine();

  if (ImGui::Button("Choose File")){
    ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgImportJob", "Choose File", ".wfinput,.json,.k", ".");
    show_job_files = true;
  }
      
  if (show_job_files) {
    if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgImportJob")) 
    {
      if (ImGuiFileDialog::Instance()->IsOk())
      {
        m_filename = ImGuiFileDialog::Instance()->GetFilePathName();
        loadRestartOptionsFromInputFile();
        if (m_edit_mode && m_job != nullptr)
          m_job->setPathFile(m_filename);
        cout << "file path name "<<m_filename<<endl;
      }
      
      ImGuiFileDialog::Instance()->Close();
      show_job_files = false;
    }
  }

  const bool supportsRestartFromModel = [&]() {
    Model &model = getApp().getActiveModel();
    if (model.getStepCount() <= 0) {
      return false;
    }
    Step* step = model.getStep(0);
    return step != nullptr && step->isImplicit() && model.getAnalysisType() == Solid3D;
  }();
  const bool supportsRestartFromInput = inputSupportsImplicit3DRestart();
  const bool supportsRestart = (!m_filename.empty() ? supportsRestartFromInput : supportsRestartFromModel);

  ImGui::Separator();
  ImGui::TextUnformatted("Restart / checkpoint");
  ImGui::TextDisabled("Available only for implicit 3D jobs (weldform_imp_3d).");

  if (!supportsRestart) {
    ImGui::BeginDisabled();
  }

  ImGui::Checkbox("Generate checkpoint", &m_checkpoint_enabled);
  if (m_checkpoint_enabled) {
    if (m_checkpoint_interval < 1)
      m_checkpoint_interval = 1;
    ImGui::InputInt("Checkpoint interval", &m_checkpoint_interval);
    if (m_checkpoint_interval < 1)
      m_checkpoint_interval = 1;
    if (ImGui::InputText("Checkpoint dir", m_checkpoint_dir_buffer.data(), m_checkpoint_dir_buffer.size())) {
      m_checkpoint_dir = m_checkpoint_dir_buffer.data();
    }
    if (ImGui::InputText("Checkpoint prefix", m_checkpoint_prefix_buffer.data(), m_checkpoint_prefix_buffer.size())) {
      m_checkpoint_prefix = m_checkpoint_prefix_buffer.data();
    }
  }

  ImGui::Separator();
  ImGui::TextUnformatted("Restart run");
  if (ImGui::InputText("Restart file", m_restart_file_buffer.data(), m_restart_file_buffer.size())) {
    m_restart_file = m_restart_file_buffer.data();
  }
  ImGui::SameLine();
  if (ImGui::Button("Browse restart")) {
    ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgRestartWfrst", "Choose Restart File", ".wfrst,.WFRST", ".");
    m_show_restart_files = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("Clear restart")) {
    m_restart_file.clear();
    m_restart_file_buffer[0] = '\0';
  }
  ImGui::TextDisabled("When set, exported as Configuration.solver.implicit.restartFile.");

  if (!supportsRestart) {
    ImGui::EndDisabled();
  }

  if (m_show_restart_files) {
    if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgRestartWfrst")) {
      if (ImGuiFileDialog::Instance()->IsOk()) {
        m_restart_file = ImGuiFileDialog::Instance()->GetFilePathName();
        std::snprintf(m_restart_file_buffer.data(), m_restart_file_buffer.size(), "%s", m_restart_file.c_str());
      }
      ImGuiFileDialog::Instance()->Close();
      m_show_restart_files = false;
    }
  }

  if (!m_edit_mode && m_filename.empty()) {
    ImGui::BeginDisabled();
  }
  if (!m_edit_mode && ImGui::Button("Create")) {
    create_entity = true;
    m_show = false;
  }
  if (!m_edit_mode && m_filename.empty()) {
    ImGui::EndDisabled();
  }
  if (m_edit_mode && m_filename.empty()) {
    ImGui::BeginDisabled();
  }
  if (m_edit_mode && ImGui::Button("Save")) {
    if (m_job != nullptr) {
      m_job->setPathFile(m_filename);
      m_job->setSolverEditionOverride(static_cast<Job::SolverEdition>(m_solver_edition));
      m_job->setCheckpointEnabled(supportsRestart ? m_checkpoint_enabled : false);
      m_job->setCheckpointInterval(m_checkpoint_interval);
      m_job->setCheckpointDir(m_checkpoint_dir);
      m_job->setCheckpointPrefix(m_checkpoint_prefix);
      m_job->setRestartFile(supportsRestart ? m_restart_file : std::string());
      m_job->applyRestartSettingsToInput();
    }
    m_edit_mode = false;
    m_job = nullptr;
    m_show = false;
  }
  if (m_edit_mode && m_filename.empty()) {
    ImGui::EndDisabled();
  }
  if (((m_edit_mode && m_job != nullptr) || !m_filename.empty()) && m_open_results) {
    ImGui::SameLine();
    if (ImGui::Button("Load Results")) {
      if (m_edit_mode && m_job != nullptr) {
        m_open_results(m_job);
      } else if (!m_filename.empty()) {
        Job tempJob;
        tempJob.setPathFile(m_filename);
        m_open_results(&tempJob);
      }
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Cancel")) {
    cancel_action = false; //REMOVE
    m_edit_mode = false;
    m_job = nullptr;
    m_show=false;
  }

  ImGui::End();
}

void JobShowDialog::Draw(){
  
  if (m_job!=nullptr){
  ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
  //create_job = false; 
  if (!ImGui::Begin("Job Progress", nullptr)){
    ImGui::End();
    return;
  }

  double now = ImGui::GetTime();
  if (m_job != m_last_job) {
    m_last_job = m_job;
    m_last_refresh_time = now;
    m_job->UpdateOutput(m_max_visible_lines);
  } else if (m_last_refresh_time < 0.0 || (now - m_last_refresh_time) >= 1.0) {
    m_last_refresh_time = now;
    m_job->UpdateOutput(m_max_visible_lines);
  }
  
  //cout << "viewing"<<endl;
  string str = m_job->getPathFile();
  ImGui::Text("Path: %s", str.c_str()  );
  ImGui::SetNextItemWidth(120.0f);
  if (ImGui::InputInt("Visible lines", &m_max_visible_lines)) {
    if (m_max_visible_lines < 1)
      m_max_visible_lines = 1;
    m_last_refresh_time = now;
    m_job->UpdateOutput(m_max_visible_lines);
  }
  

  str = m_job->getLog();

  const double simTime = m_job->getExpectedSimTime();
  const fs::path resultsPath = m_job->getResultsFilePath();
  const double currentTime = m_job->getCurrentResultTime();
  const float progress = m_job->getEstimatedProgress();

  ImGui::Separator();
  ImGui::Text("Estimated progress");
  ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f));
  if (simTime > 0.0) {
    ImGui::Text("%.4f / %.4f time", currentTime, simTime);
  } else {
    ImGui::TextUnformatted("No se pudo estimar el tiempo total del step.");
  }
  if (!resultsPath.empty()) {
    ImGui::Text("Results: %s", resultsPath.string().c_str());
  } else {
    ImGui::TextUnformatted("Results: waiting for *.wfresult");
  }
  ImGui::Separator();

  //ImGui::Text("State %s", str.c_str()  );  

  ImGui::BeginChild("JobLogBuffer", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true, ImGuiWindowFlags_HorizontalScrollbar);
  ImGui::PushTextWrapPos();
  ImGui::TextUnformatted(str.c_str());
  ImGui::PopTextWrapPos();
  ImGui::SetScrollHereY(1.0f);
  ImGui::EndChild();
  
  if (ImGui::Button("Close")){
    m_last_refresh_time = -1.0;
    m_show=false;
  }
  ImGui::SameLine();
  if (ImGui::Button("Stop Run")) {
    if (m_job != nullptr) {
      m_job->Stop();
      m_job->UpdateOutput(m_max_visible_lines);
    }
  }
  ImGui::SameLine();
  const bool canLoadResults = m_open_results && !resultsPath.empty();
  if (!canLoadResults)
    ImGui::BeginDisabled();
  if (ImGui::Button("Load Results")) {
    if (!resultsPath.empty()) {
      // cout << "[job-dialog] load results requested for " << resultsPath.string() << endl;
      m_job->UpdateOutput(m_max_visible_lines);
      m_open_results(m_job);
      m_last_refresh_time = -1.0;
      m_show = false;
    } else {
      // cout << "[job-dialog] load results requested but no results json detected yet" << endl;
    }
  }
  if (!canLoadResults)
    ImGui::EndDisabled();

  ImGui::End();
  }
}
/*
void JobShowDialog::Draw(const char* title, bool* p_open, Job *job){


}
*/

// Job ShowCreateJobDialog(bool* p_open, JobDialog *jobdlg, bool* ret){
  // Job job;

  // jobdlg->Draw("Job", p_open);
  
  // return job;
// }

// //SAME DIALOG FROM CREATE AND EDIT MATERIAL
// // IS BASICALLY THE SAME 
// struct MaterialDialog{
  
  // //void    AddLog(const char* fmt, ...);
  // double m_density_const; //TODO; CHANGE TO VECTOR (TEMP FUNCTION)
  // double m_elastic_const;
  // double m_poisson_const;
  
  // bool cancel_action;
  // bool create_material;
  
  // const bool & isJobCreated()const{return create_material;}
  // void   Draw(const char* title, bool* p_open = NULL, Material_* mat = NULL);  
// };

// //Returns true if NEW material is created or if changes are saved, if no
// //if no material is created, pointer is null
// Material_ ShowCreateMaterialDialog(bool* p_open, MaterialDialog *, bool* ret);
// bool ShowEditMaterialDialog(bool* p_open, MaterialDialog *, Material_ *);


//template  void EntityDialog<Job>:: Draw(const char* title, bool* p_open, Job *mat);
