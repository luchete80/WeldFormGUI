
#include "job_dialog.h"
#include "io/InputWriter.h"
#include "App.h"
#include "model/Step.h"
#include <iostream>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "ImGuiFileDialog.h"
#include <filesystem>
#include <fstream>
#include <cmath>
#include <vector>
#include <nlohmann/json.hpp>

using namespace std;
namespace fs = std::filesystem;
using json = nlohmann::json;

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

fs::path findResultsJsonForJob(Job &job) {
  const std::string &jobPath = job.getPathFile();
  if (jobPath.empty())
    return {};

  fs::path runPath(jobPath);
  fs::path runDir = runPath.parent_path();
  std::string stem = runPath.stem().string();

  std::vector<fs::path> candidates;
  if (stem.size() >= 4 && stem.substr(stem.size() - 4) == "_run")
    candidates.push_back(runDir / (stem.substr(0, stem.size() - 4) + "_res.json"));
  candidates.push_back(runDir / (stem + "_res.json"));
  candidates.push_back(runDir / "modelo_res.json");

  for (const auto &candidate : candidates) {
    if (fs::exists(candidate))
      return candidate;
  }

  return {};
}

double getLastResultTime(const fs::path &resultsPath) {
  if (resultsPath.empty() || !fs::exists(resultsPath))
    return 0.0;

  std::ifstream file(resultsPath);
  if (!file.is_open())
    return 0.0;

  json data;
  try {
    file >> data;
  } catch (...) {
    return 0.0;
  }

  if (!data.contains("vtk_files") || !data["vtk_files"].is_array())
    return 0.0;

  double lastTime = 0.0;
  for (const auto &entry : data["vtk_files"]) {
    if (!entry.contains("time"))
      continue;
    try {
      lastTime = entry["time"].get<double>();
    } catch (...) {
      continue;
    }
  }

  return lastTime;
}

double expectedSimTime(Model &model) {
  if (model.getStepCount() <= 0)
    return 0.0;

  Step *step = model.getStep(0);
  if (step == nullptr || step->m_simTime <= 0.0)
    return 0.0;

  return step->m_simTime;
}
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

  if (!m_edit_mode && ImGui::Button("From Model")){
    Model &model = getApp().getActiveModel();
    if (model.getHasName()) {
      fs::path input_path = modelOutputPath(model, modelStem(model) + "_run.json");
      m_filename = input_path.string();
      InputWriter writer(&model);
      writer.writeToFile(input_path.string());
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
    ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgImportJob", "Choose File", ".json,.k", ".");
    show_job_files = true;
  }
      
  if (show_job_files) {
    if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgImportJob")) 
    {
      if (ImGuiFileDialog::Instance()->IsOk())
      {
        m_filename = ImGuiFileDialog::Instance()->GetFilePathName();
        if (m_edit_mode && m_job != nullptr)
          m_job->setPathFile(m_filename);
        else
          create_entity = true;
        cout << "file path name "<<m_filename<<endl;
      }
      
      ImGuiFileDialog::Instance()->Close();
      show_job_files = false;
    }
  }

  if (!m_edit_mode && ImGui::Button("Create")) {
    create_entity = true;
    m_show = false;
  }
  if (m_edit_mode && ImGui::Button("Save")) {
    if (m_job != nullptr)
      m_job->setPathFile(m_filename);
    m_edit_mode = false;
    m_job = nullptr;
    m_show = false;
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

  Model &model = getApp().getActiveModel();
  const double simTime = expectedSimTime(model);
  const fs::path resultsPath = findResultsJsonForJob(*m_job);
  const double currentTime = getLastResultTime(resultsPath);
  const float progress = simTime > 0.0
    ? std::min(1.0f, static_cast<float>(currentTime / simTime))
    : 0.0f;

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
    ImGui::TextUnformatted("Results: waiting for *_res.json");
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
