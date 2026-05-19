#ifndef WFGUI_WORKFLOW_API_H
#define WFGUI_WORKFLOW_API_H

#include "../App/App.h"
#include "../geom/Geom.h"
#include "../graphicmesh/GraphicMesh.h"
#include "../io/InputWriter.h"
#include "../io/ModelWriter.h"
#include "../model/BoundaryCondition.h"
#include "../model/Material.h"
#include "../model/Mesh.h"
#include "../model/Node.h"
#include "../model/Part.h"
#include "../model/Step.h"

#include <gmsh.h>
#include <vtkNew.h>
#include <vtkTransform.h>
#include <vtkTransformFilter.h>

#include <set>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <system_error>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

void syncScriptModelWithEditor(Model* model);

inline Model* get_active_model();

namespace wfgui {
namespace workflow {

inline bool point_in_box(double x,
                         double y,
                         double z,
                         double xmin,
                         double ymin,
                         double zmin,
                         double xmax,
                         double ymax,
                         double zmax)
{
  const double lo_x = (xmin < xmax) ? xmin : xmax;
  const double hi_x = (xmin < xmax) ? xmax : xmin;
  const double lo_y = (ymin < ymax) ? ymin : ymax;
  const double hi_y = (ymin < ymax) ? ymax : ymin;
  const double lo_z = (zmin < zmax) ? zmin : zmax;
  const double hi_z = (zmin < zmax) ? zmax : zmin;

  return x >= lo_x && x <= hi_x &&
         y >= lo_y && y <= hi_y &&
         z >= lo_z && z <= hi_z;
}

inline int next_node_set_id(Mesh* mesh)
{
  if (mesh == nullptr)
    return 0;

  int max_id = -1;
  for (int i = 0; i < mesh->getNodeSetCount(); ++i) {
    if (mesh->getNodeSet(i).getId() > max_id)
      max_id = mesh->getNodeSet(i).getId();
  }
  return max_id + 1;
}

inline int next_element_set_id(Mesh* mesh)
{
  if (mesh == nullptr)
    return 0;

  int max_id = -1;
  for (int i = 0; i < mesh->getElementSetCount(); ++i) {
    if (mesh->getElementSet(i).getId() > max_id)
      max_id = mesh->getElementSet(i).getId();
  }
  return max_id + 1;
}

inline int store_node_set(Mesh* mesh, NodeSet& node_set, int set_id)
{
  node_set.setEntityId(set_id);
  mesh->addNodeSet(node_set);
  return mesh->getNodeSet(mesh->getNodeSetCount() - 1).getId();
}

inline int store_element_set(Mesh* mesh, ElementSet& element_set, int set_id)
{
  element_set.setEntityId(set_id);
  mesh->addElementSet(element_set);
  return mesh->getElementSet(mesh->getElementSetCount() - 1).getId();
}

inline std::string to_lower_copy(std::string value)
{
  for (char& ch : value) {
    if (ch >= 'A' && ch <= 'Z')
      ch = static_cast<char>(ch - 'A' + 'a');
  }
  return value;
}

inline std::filesystem::path executable_directory()
{
#ifdef _WIN32
  std::vector<char> buffer(MAX_PATH, '\0');
  DWORD length = 0;
  while (true) {
    length = GetModuleFileNameA(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0)
      return {};
    if (length < buffer.size() - 1)
      break;
    buffer.resize(buffer.size() * 2, '\0');
  }
  return std::filesystem::path(std::string(buffer.data(), length)).parent_path();
#else
  std::vector<char> buffer(1024, '\0');
  while (true) {
    const ssize_t length = ::readlink("/proc/self/exe", buffer.data(), buffer.size() - 1);
    if (length < 0)
      return {};
    if (length < static_cast<ssize_t>(buffer.size() - 1))
      return std::filesystem::path(std::string(buffer.data(), static_cast<std::size_t>(length))).parent_path();
    buffer.resize(buffer.size() * 2, '\0');
  }
#endif
}

inline std::vector<std::filesystem::path> solver_search_directories(const std::filesystem::path& input_dir)
{
  namespace fs = std::filesystem;
  std::vector<fs::path> dirs;
  const fs::path exe_dir = executable_directory();
  const fs::path cwd = fs::current_path();

  if (!exe_dir.empty()) {
    dirs.push_back(exe_dir / "solvers");
    dirs.push_back(exe_dir.parent_path() / "solvers");
  }

  dirs.push_back(cwd / "solvers");
  dirs.push_back(cwd.parent_path() / "WeldFormGUI" / "solvers");

  if (!input_dir.empty())
    dirs.push_back(input_dir / "solvers");

  std::vector<fs::path> unique_dirs;
  std::set<std::string> seen;
  for (const fs::path& dir : dirs) {
    const std::string key = dir.lexically_normal().string();
    if (seen.insert(key).second)
      unique_dirs.push_back(dir.lexically_normal());
  }
  return unique_dirs;
}

inline std::vector<std::filesystem::path> runtime_search_directories()
{
  namespace fs = std::filesystem;
  std::vector<fs::path> dirs;
  const fs::path exe_dir = executable_directory();
  const fs::path cwd = fs::current_path();

  if (!exe_dir.empty()) {
    dirs.push_back(exe_dir);
    dirs.push_back(exe_dir / "solvers");
    dirs.push_back(exe_dir.parent_path());
    dirs.push_back(exe_dir.parent_path() / "solvers");
  }

  dirs.push_back(cwd);
  dirs.push_back(cwd / "solvers");

  std::vector<fs::path> unique_dirs;
  std::set<std::string> seen;
  for (const fs::path& dir : dirs) {
    const std::string key = dir.lexically_normal().string();
    if (seen.insert(key).second)
      unique_dirs.push_back(dir.lexically_normal());
  }
  return unique_dirs;
}

inline std::filesystem::path resolve_runtime_executable_path(
    const std::vector<std::string>& candidates)
{
  namespace fs = std::filesystem;

  for (const fs::path& dir : runtime_search_directories()) {
    for (const std::string& candidate_name : candidates) {

#ifdef _WIN32
      fs::path candidate_exe = dir / (candidate_name + ".exe");

      if (fs::exists(candidate_exe))
        return candidate_exe;
#endif

      fs::path candidate = dir / candidate_name;

      if (fs::exists(candidate))
        return candidate;
    }
  }

  return {};
}

inline std::string active_model_stem(Model& model)
{
  namespace fs = std::filesystem;
  fs::path model_name(model.getName());
  std::string stem = model_name.stem().string();
  if (stem.empty())
    stem = model_name.filename().string();
  if (stem.empty())
    stem = "model";
  return stem;
}

inline std::filesystem::path active_model_output_path(Model& model, const std::string& filename)
{
  return std::filesystem::path(model.getBaseDir()) / filename;
}

inline bool is_2d_analysis_type(AnalysisType analysis_type)
{
  return analysis_type == PlaneStress2D ||
         analysis_type == PlaneStrain2D ||
         analysis_type == Axisymmetric2D;
}

inline int curve_node_count_from_element_size(int curve_tag, double element_size)
{
  if (element_size <= 0.0)
    return 2;

  double xmin = 0.0;
  double ymin = 0.0;
  double zmin = 0.0;
  double xmax = 0.0;
  double ymax = 0.0;
  double zmax = 0.0;
  gmsh::model::getBoundingBox(1, curve_tag, xmin, ymin, zmin, xmax, ymax, zmax);

  const double dx = xmax - xmin;
  const double dy = ymax - ymin;
  const double dz = zmax - zmin;
  const double approx_length = std::sqrt(dx * dx + dy * dy + dz * dz);
  return std::max(2, static_cast<int>(std::ceil(approx_length / element_size)) + 1);
}

inline void apply_mesh_size_to_current_gmsh_model(double element_size)
{
  if (element_size <= 0.0)
    return;

  std::vector<std::pair<int, int>> point_entities;
  gmsh::model::getEntities(point_entities, 0);
  for (const auto& entity : point_entities) {
    gmsh::model::mesh::setSize({entity}, element_size);
  }

  std::vector<std::pair<int, int>> curve_entities;
  gmsh::model::getEntities(curve_entities, 1);
  for (const auto& entity : curve_entities) {
    gmsh::model::mesh::setTransfiniteCurve(
      entity.second, curve_node_count_from_element_size(entity.second, element_size));
  }
}

inline int find_part_index(Model* model, Part* part)
{
  if (model == nullptr || part == nullptr)
    return -1;

  for (int i = 0; i < model->getPartCount(); ++i) {
    if (model->getPart(i) == part)
      return i;
  }
  return -1;
}

inline int system_exit_code(int status_code)
{
#ifdef _WIN32
  return status_code;
#else
  if (status_code == -1)
    return -1;
  if (!WIFEXITED(status_code))
    return status_code;
  return WEXITSTATUS(status_code);
#endif
}

inline bool active_model_is_implicit()
{
  Model* model = get_active_model();
  if (model == nullptr || model->getStepCount() <= 0 || model->getStep(0) == nullptr)
    return false;
  return model->getStep(0)->isImplicit();
}

inline std::string preferred_solver_binary_name()
{
  const std::string base_name = active_model_is_implicit() ? "weldform_imp" : "weldform_exp";
  const char* edition_env = std::getenv("WELDFORM_SOLVER_EDITION");
  const std::string edition = edition_env != nullptr ? to_lower_copy(edition_env) : "";

  if (edition == "full")
    return base_name;
  if (edition == "std" || edition == "student")
    return base_name + "_std";
  return base_name;
}

inline std::filesystem::path resolve_solver_executable_path(const std::filesystem::path& input_path)
{
  namespace fs = std::filesystem;
  const fs::path input_dir = input_path.has_parent_path() ? input_path.parent_path() : fs::path(".");
  const std::string preferred = preferred_solver_binary_name();
  std::vector<std::string> candidates = {preferred};

  if (preferred.size() >= 4 && preferred.substr(preferred.size() - 4) == "_std")
    candidates.push_back(preferred.substr(0, preferred.size() - 4));
  else
    candidates.push_back(preferred + "_std");

  for (const fs::path& dir : solver_search_directories(input_dir)) {
    for (const std::string& binary : candidates) {
#ifdef _WIN32
      const fs::path candidate_exe = dir / (binary + ".exe");
      if (fs::exists(candidate_exe))
        return candidate_exe;
#endif
      const fs::path candidate = dir / binary;
      if (fs::exists(candidate))
        return candidate;
    }
  }

  return {};
}

inline int launch_detached_process(const std::filesystem::path& executable_path,
                                   const std::filesystem::path& input_path)
{
#ifdef _WIN32
  STARTUPINFOA si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  ZeroMemory(&pi, sizeof(pi));
  si.cb = sizeof(si);

  std::string command_line = "\"" + executable_path.string() + "\" \"" + input_path.string() + "\"";
  std::vector<char> mutable_command(command_line.begin(), command_line.end());
  mutable_command.push_back('\0');
  std::string working_dir = executable_path.has_parent_path() ? executable_path.parent_path().string() : std::string(".");

  const BOOL ok = CreateProcessA(
    nullptr,
    mutable_command.data(),
    nullptr,
    nullptr,
    FALSE,
    CREATE_NEW_PROCESS_GROUP | DETACHED_PROCESS,
    nullptr,
    working_dir.c_str(),
    &si,
    &pi
  );

  if (!ok)
    return -1;

  const int pid = static_cast<int>(pi.dwProcessId);
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  return pid;
#else
  pid_t pid = fork();
  if (pid < 0)
    return -1;
  if (pid == 0) {
    setsid();
    const std::string working_dir = executable_path.has_parent_path() ? executable_path.parent_path().string() : std::string(".");
    (void)chdir(working_dir.c_str());
    execl(executable_path.c_str(), executable_path.c_str(), input_path.c_str(), static_cast<char*>(nullptr));
    _exit(127);
  }
  return static_cast<int>(pid);
#endif
}

} // namespace workflow
} // namespace wfgui

inline Model* get_active_model()
{
  return &getApp().getActiveModel();
}

inline Model* create_active_model(const std::string& model_name = "PythonModel")
{
  Model* model = new Model();
  model->setName(model_name);
  syncScriptModelWithEditor(model);
  std::cout << "Created model from Python: " << model_name << std::endl;
  getApp().Update();
  return model;
}

inline void add_part_to_active_model(Part* part)
{
  if (part == nullptr)
    return;

  Model* model = get_active_model();
  if (model != nullptr) {
    model->addPart(part);

    if (part->getGeom() != nullptr) {
      vtkOCCTGeom* visual = getApp().getVisualForPart(part);
      if (visual == nullptr) {
        visual = getApp().registerGeometry(part->getGeom());
        getApp().registerPartVisual(part, visual);
      }
    }

    getApp().Update();
  }
}

inline void request_view_update()
{
  getApp().Update();
}

inline std::string get_active_model_file_path()
{
  Model* model = get_active_model();
  if (model == nullptr)
    return "";
  return model->getFilePath();
}

inline void add_material_to_active_model(Material_* material)
{
  if (material == nullptr)
    return;

  Model* model = get_active_model();
  if (model != nullptr)
    model->addMaterial(material);
}

inline void set_active_model_analysis_type(int analysis_type)
{
  Model* model = get_active_model();
  if (model == nullptr)
    return;

  switch (analysis_type) {
    case PlaneStress2D:
    case PlaneStrain2D:
    case Axisymmetric2D:
    case Solid3D:
      model->setAnalysisType(static_cast<AnalysisType>(analysis_type));
      break;
    default:
      break;
  }
}

inline int get_active_model_analysis_type()
{
  Model* model = get_active_model();
  if (model == nullptr)
    return Solid3D;
  return static_cast<int>(model->getAnalysisType());
}

inline void set_active_model_contact_properties(double penalty_factor,
                                                double gap_penalty_scale,
                                                bool use_gap_penalty = true,
                                                double static_friction = 0.0,
                                                double heat_cond_coeff = 0.5,
                                                bool heat_conductance = false,
                                                double max_penet_ratio = 0.05,
                                                double max_accel = 100000.0)
{
  Model* model = get_active_model();
  if (model == nullptr)
    return;

  ContactProperties& contact = model->contactProps();
  contact.penaltyFactor = penalty_factor;
  contact.gapPenaltyScale = gap_penalty_scale;
  contact.useGapPenalty = use_gap_penalty;
  contact.fricCoeffStatic = static_friction;
  contact.heatCondCoeff = heat_cond_coeff;
  contact.heatConductance = heat_conductance;
  contact.maxPenetRatio = max_penet_ratio;
  contact.maxAccel = max_accel;
}

inline bool save_active_model(const std::string& requested_path)
{
  Model* model = get_active_model();
  if (model == nullptr)
    return false;

  namespace fs = std::filesystem;
  fs::path output_path(requested_path);
  if (output_path.extension().empty())
    output_path += ".wfmodel";
  output_path = fs::absolute(output_path);

  std::error_code ec;
  if (output_path.has_parent_path())
    fs::create_directories(output_path.parent_path(), ec);

  model->setName(output_path.filename().string());
  model->setFilePath(output_path.string());
  model->setNoSaveAs();

  ModelWriter writer(*model);
  writer.writeToFile(output_path.string());
  return true;
}

inline bool write_active_model_input(const std::string& requested_path)
{
  Model* model = get_active_model();
  if (model == nullptr)
    return false;

  namespace fs = std::filesystem;
  fs::path output_path(requested_path);
  if (output_path.extension().empty())
    output_path += ".json";
  output_path = fs::absolute(output_path);

  std::error_code ec;
  if (output_path.has_parent_path())
    fs::create_directories(output_path.parent_path(), ec);

  InputWriter writer(model);
  writer.writeToFile(output_path.string());
  return true;
}

inline std::string default_active_model_output_stem()
{
  namespace fs = std::filesystem;
  Model* model = get_active_model();
  if (model == nullptr)
    return fs::absolute("model").string();

  if (!model->getFilePath().empty()) {
    fs::path path(model->getFilePath());
    if (path.has_extension())
      path.replace_extension("");
    return fs::absolute(path).string();
  }

  std::string name = model->getName();
  if (name.empty())
    name = "model";
  fs::path path(name);
  if (path.has_extension())
    path.replace_extension("");
  return fs::absolute(path).string();
}

inline int run_active_model_solver(const std::string& requested_input_path = "")
{
  namespace fs = std::filesystem;
  fs::path input_path;
  if (requested_input_path.empty())
    input_path = fs::path(default_active_model_output_stem() + ".json");
  else
    input_path = fs::path(requested_input_path);

  if (!write_active_model_input(input_path.string()))
    return -1;

  input_path = fs::absolute(input_path);
  const fs::path solver_path = wfgui::workflow::resolve_solver_executable_path(input_path);
  if (solver_path.empty())
    return -1;

  return wfgui::workflow::launch_detached_process(solver_path, input_path);
}

inline Step* ensure_analysis_step(const std::string& step_name = "Step-1")
{
  Model* model = get_active_model();
  if (model == nullptr)
    return nullptr;

  if (model->getStepCount() > 0 && model->getStep(0) != nullptr) {
    Step* step = model->getStep(0);
    step->setName(step_name.c_str());
    return step;
  }

  Step* step = new Step();
  step->setName(step_name.c_str());
  model->addStep(step);
  return step;
}

inline Step* create_implicit_step(const std::string& step_name = "Step-1",
                                  double sim_time = 1.0,
                                  double out_time = 0.1,
                                  const std::string& implicit_type = "Picard",
                                  int max_iter = 200,
                                  double vel_tol = 5.0e-2,
                                  double press_tol = 10.0,
                                  double force_tol = 10.0,
                                  double div_tol = 1.0,
                                  double omega_v = 0.4,
                                  double omega_p = 0.1,
                                  double time_step_growth_factor = 1.2,
                                  int nproc = 1)
{
  Step* step = ensure_analysis_step(step_name);
  if (step == nullptr)
    return nullptr;

  step->setStepType(ImplicitStep);
  step->m_simTime = sim_time;
  step->m_outTime = out_time;
  step->m_implicitType = implicit_type;
  step->m_maxIter = max_iter;
  step->m_velTol = vel_tol;
  step->m_pressTol = press_tol;
  step->m_forceTol = force_tol;
  step->m_divTol = div_tol;
  step->m_omegaV = omega_v;
  step->m_omegaP = omega_p;
  step->m_timeStepGrowthFactor = time_step_growth_factor;
  step->m_nproc = nproc;
  return step;
}

inline Part* create_empty_mesh_part()
{
  Mesh* mesh = new Mesh();
  return new Part(mesh);
}

inline Part* create_rectangle_part(double dx,
                                   double dy,
                                   double ox = 0.0,
                                   double oy = 0.0,
                                   double oz = 0.0)
{
  Geom* geom = new Geom();
  geom->LoadRectangle(dx, dy, ox, oy, oz);
  return new Part(geom);
}

inline Part* import_step_part(const std::string& step_path)
{
  Geom* geom = new Geom();
  if (!geom->LoadSTEP(step_path)) {
    delete geom;
    return nullptr;
  }
  return new Part(geom);
}

inline Part* import_step_part_at(const std::string& step_path,
                                 double origin_x,
                                 double origin_y,
                                 double origin_z)
{
  Geom* geom = new Geom();
  if (!geom->LoadSTEP(step_path, origin_x, origin_y, origin_z)) {
    delete geom;
    return nullptr;
  }
  return new Part(geom);
}

inline Part* import_bdf_part(const std::string& bdf_path)
{
  namespace fs = std::filesystem;
  Part* part = new Part();
  part->generateMeshFromNastranFile(bdf_path);
  part->setName(fs::path(bdf_path).stem().string().c_str());
  return part;
}

inline void translate_part(Part* part, double dx, double dy, double dz)
{
  if (part == nullptr)
    return;

  if (std::abs(dx) <= 1.0e-12 && std::abs(dy) <= 1.0e-12 && std::abs(dz) <= 1.0e-12)
    return;

  vtkOCCTGeom* visual = getApp().getVisualForPart(part);
  if (visual != nullptr && visual->getPolydata() != nullptr) {
    vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
    transform->Translate(dx, dy, dz);

    vtkNew<vtkTransformFilter> transform_filter;
    transform_filter->SetInputData(visual->getPolydata());
    transform_filter->SetTransform(transform);
    transform_filter->Update();
    visual->getPolydata()->ShallowCopy(transform_filter->GetOutput());
  }

  if (part->getGeom() != nullptr)
    part->getGeom()->Move(dx, dy, dz);

  if (GraphicMesh* graphic_mesh = getApp().getGraphicMeshFromPart(part))
    graphic_mesh->Translate(dx, dy, dz);

  getApp().Update();
}

inline Part* import_bdf_part_at(const std::string& bdf_path,
                                double origin_x,
                                double origin_y,
                                double origin_z)
{
  Part* part = import_bdf_part(bdf_path);
  if (part == nullptr)
    return nullptr;

  translate_part(part, origin_x, origin_y, origin_z);
  return part;
}

inline bool mesh_part_geometry(Part* part, double element_size = -1.0)
{
  namespace fs = std::filesystem;

  Model* model = get_active_model();
  if (model == nullptr || part == nullptr || part->getGeom() == nullptr)
    return false;

  const int part_index = wfgui::workflow::find_part_index(model, part);
  if (part_index < 0)
    return false;

  const double resolved_element_size =
    (element_size > 0.0) ? element_size : model->getElementSize();

  Geom* geom = part->getGeom();
  const fs::path step_path = wfgui::workflow::active_model_output_path(
    *model,
    wfgui::workflow::active_model_stem(*model) + "_part_" + std::to_string(part_index) + ".step");

  std::error_code ec;
  if (step_path.has_parent_path())
    fs::create_directories(step_path.parent_path(), ec);

  geom->setFileName(step_path.string());
  geom->ExportSTEP();

  if (part->isMeshed())
    part->deleteMesh();

  const AnalysisType analysis_type = model->getAnalysisType();
  const bool is_2d_analysis = wfgui::workflow::is_2d_analysis_type(analysis_type);
  const bool is_rigid_part = (part->getType() == Rigid);

  if (is_2d_analysis && !is_rigid_part) {
    const fs::path bdf_export_path = wfgui::workflow::active_model_output_path(
      *model,
      wfgui::workflow::active_model_stem(*model) + "_part_" + std::to_string(part_index) + ".bdf");
    const fs::path remesh_log_path = wfgui::workflow::active_model_output_path(
      *model,
      wfgui::workflow::active_model_stem(*model) + "_part_" + std::to_string(part_index) + "_mesh_adapt.tmp");
    const fs::path remesher_path = wfgui::workflow::resolve_runtime_executable_path(
      {"mesh-adapt", "mesh-adapt_std"});
    const std::string remesher = remesher_path.empty() ? std::string("mesh-adapt") : remesher_path.string();

    const fs::path previous_cwd = fs::current_path();
    fs::path output_bdf_path = previous_cwd / "output_smoothed.bdf";
    if (step_path.has_parent_path())
      fs::current_path(step_path.parent_path(), ec);

    output_bdf_path = fs::current_path() / "output_smoothed.bdf";
    std::filesystem::remove(output_bdf_path, ec);
    const std::string remesh_cmd =
      "\"" + remesher + "\" \"" + step_path.string() + "\" " +
      std::to_string(resolved_element_size) + " > \"" + remesh_log_path.string() + "\" 2>&1";
    const int remesh_status = std::system(remesh_cmd.c_str());
    fs::current_path(previous_cwd, ec);

    if (wfgui::workflow::system_exit_code(remesh_status) != 0)
      return false;
    if (!fs::exists(output_bdf_path))
      return false;

    fs::copy_file(output_bdf_path, bdf_export_path, fs::copy_options::overwrite_existing, ec);
    if (ec)
      return false;

    part->generateMeshFromNastranFile(bdf_export_path.string());
    return true;
  }

  gmsh::clear();
  gmsh::model::add("wf_script_mesh");
  std::vector<std::pair<int, int>> entities;
  gmsh::model::occ::importShapes(step_path.string(), entities);
  gmsh::model::occ::synchronize();
  const int gmsh_dim = gmsh::model::getDimension();

  wfgui::workflow::apply_mesh_size_to_current_gmsh_model(resolved_element_size);

  if (is_2d_analysis && is_rigid_part) {
    gmsh::model::mesh::generate(1);
  } else {
    gmsh::model::getEntities(entities);
    for (const auto& entity : entities) {
      if (entity.first == 2) {
        gmsh::model::mesh::setTransfiniteSurface(entity.second);
        gmsh::model::mesh::setRecombine(2, entity.second);
      }
    }
    if (gmsh_dim > -1)
      gmsh::model::mesh::generate(gmsh_dim);
  }

  const fs::path mesh_path = wfgui::workflow::active_model_output_path(
    *model,
    wfgui::workflow::active_model_stem(*model) + "_part_" + std::to_string(part_index) + ".msh");
  gmsh::write(mesh_path.string().c_str());
  part->generateMesh();
  return true;
}

inline Part* import_and_mesh_step_part(const std::string& step_path,
                                       double element_size = -1.0,
                                       double origin_x = 0.0,
                                       double origin_y = 0.0,
                                       double origin_z = 0.0)
{
  Part* part = import_step_part_at(step_path, origin_x, origin_y, origin_z);
  if (part == nullptr)
    return nullptr;

  add_part_to_active_model(part);
  if (!mesh_part_geometry(part, element_size))
    return nullptr;
  return part;
}

inline Material_* create_hollomon_material(double young_modulus,
                                           double poisson_ratio,
                                           double density,
                                           double yield_stress0,
                                           double strength_k,
                                           double hardening_n,
                                           double strain_min = 0.0,
                                           double strain_max = 0.65)
{
  Material_* material = new Material_(Elastic_(young_modulus, poisson_ratio));
  material->setDensityConstant(density);
  material->yieldStress0 = yield_stress0;
  material->strRange = {strain_min, strain_max};
  material->e_min = strain_min;
  material->e_max = strain_max;
  material->InitHollomon(material->Elastic(), yield_stress0, strength_k, hardening_n);
  material->m_plastic = new Hollomon(strength_k, hardening_n);
  material->m_isplastic = true;
  return material;
}

inline void add_velocity_bc_to_node_set(int node_set_id,
                                        double vx,
                                        double vy,
                                        double vz,
                                        bool dof_x = true,
                                        bool dof_y = true,
                                        bool dof_z = true)
{
  Model* model = get_active_model();
  if (model == nullptr)
    return;

  BoundaryCondition* bc = new BoundaryCondition(
    VelocityBC,
    ApplyToNodeSet,
    node_set_id,
    make_double3(vx, vy, vz)
  );
  bc->setDofMask(dof_x, dof_y, dof_z);
  model->addBoundaryCondition(bc);
}

inline void add_velocity_bc_to_part(Part* part,
                                    double vx,
                                    double vy,
                                    double vz,
                                    bool dof_x = true,
                                    bool dof_y = true,
                                    bool dof_z = true)
{
  Model* model = get_active_model();
  if (model == nullptr || part == nullptr)
    return;

  BoundaryCondition* bc = new BoundaryCondition(
    VelocityBC,
    ApplyToPart,
    part->getId(),
    make_double3(vx, vy, vz)
  );
  bc->setDofMask(dof_x, dof_y, dof_z);
  model->addBoundaryCondition(bc);
}

inline void add_displacement_bc_to_node_set(int node_set_id,
                                            double ux,
                                            double uy,
                                            double uz,
                                            bool dof_x = true,
                                            bool dof_y = true,
                                            bool dof_z = true)
{
  Model* model = get_active_model();
  if (model == nullptr)
    return;

  BoundaryCondition* bc = new BoundaryCondition(
    DisplacementBC,
    ApplyToNodeSet,
    node_set_id,
    make_double3(ux, uy, uz)
  );
  bc->setDofMask(dof_x, dof_y, dof_z);
  model->addBoundaryCondition(bc);
}

inline void add_displacement_bc_to_part(Part* part,
                                        double ux,
                                        double uy,
                                        double uz,
                                        bool dof_x = true,
                                        bool dof_y = true,
                                        bool dof_z = true)
{
  Model* model = get_active_model();
  if (model == nullptr || part == nullptr)
    return;

  BoundaryCondition* bc = new BoundaryCondition(
    DisplacementBC,
    ApplyToPart,
    part->getId(),
    make_double3(ux, uy, uz)
  );
  bc->setDofMask(dof_x, dof_y, dof_z);
  model->addBoundaryCondition(bc);
}

inline void add_fixed_bc_to_node_set(int node_set_id,
                                     bool fix_x = true,
                                     bool fix_y = true,
                                     bool fix_z = true)
{
  add_displacement_bc_to_node_set(node_set_id, 0.0, 0.0, 0.0, fix_x, fix_y, fix_z);
}

inline void add_fixed_bc_to_part(Part* part,
                                 bool fix_x = true,
                                 bool fix_y = true,
                                 bool fix_z = true)
{
  add_displacement_bc_to_part(part, 0.0, 0.0, 0.0, fix_x, fix_y, fix_z);
}

inline int add_node_set_from_indices(Mesh* mesh,
                                     const std::string& name,
                                     const std::vector<int>& node_indices,
                                     int set_id = -1)
{
  if (mesh == nullptr)
    return -1;

  NodeSet node_set(mesh);
  node_set.setLabel(name);
  for (int index : node_indices) {
    if (index < 0 || index >= mesh->getNodeCount())
      continue;
    node_set.add(mesh->getNode(index));
  }

  const int resolved_id = (set_id >= 0) ? set_id : wfgui::workflow::next_node_set_id(mesh);
  return wfgui::workflow::store_node_set(mesh, node_set, resolved_id);
}

inline int add_element_set_from_indices(Mesh* mesh,
                                        const std::string& name,
                                        const std::vector<int>& element_indices,
                                        int set_id = -1)
{
  if (mesh == nullptr)
    return -1;

  ElementSet element_set(mesh);
  element_set.setLabel(name);
  for (int index : element_indices) {
    if (index < 0 || index >= mesh->getElemCount())
      continue;
    element_set.add(mesh->getElem(index));
  }

  const int resolved_id = (set_id >= 0) ? set_id : wfgui::workflow::next_element_set_id(mesh);
  return wfgui::workflow::store_element_set(mesh, element_set, resolved_id);
}

inline int add_node_set_from_ids(Mesh* mesh,
                                 const std::string& name,
                                 const std::vector<int>& node_ids,
                                 int set_id = -1)
{
  if (mesh == nullptr)
    return -1;

  const std::set<int> wanted(node_ids.begin(), node_ids.end());
  NodeSet node_set(mesh);
  node_set.setLabel(name);
  for (int i = 0; i < mesh->getNodeCount(); ++i) {
    Node* node = mesh->getNode(i);
    if (node != nullptr && wanted.find(node->getId()) != wanted.end())
      node_set.add(node);
  }

  const int resolved_id = (set_id >= 0) ? set_id : wfgui::workflow::next_node_set_id(mesh);
  return wfgui::workflow::store_node_set(mesh, node_set, resolved_id);
}

inline int add_element_set_from_ids(Mesh* mesh,
                                    const std::string& name,
                                    const std::vector<int>& element_ids,
                                    int set_id = -1)
{
  if (mesh == nullptr)
    return -1;

  const std::set<int> wanted(element_ids.begin(), element_ids.end());
  ElementSet element_set(mesh);
  element_set.setLabel(name);
  for (int i = 0; i < mesh->getElemCount(); ++i) {
    Element* element = mesh->getElem(i);
    if (element != nullptr && wanted.find(element->getId()) != wanted.end())
      element_set.add(element);
  }

  const int resolved_id = (set_id >= 0) ? set_id : wfgui::workflow::next_element_set_id(mesh);
  return wfgui::workflow::store_element_set(mesh, element_set, resolved_id);
}

inline std::vector<int> find_node_indices_in_box(Mesh* mesh,
                                                 double xmin,
                                                 double ymin,
                                                 double zmin,
                                                 double xmax,
                                                 double ymax,
                                                 double zmax)
{
  std::vector<int> indices;
  if (mesh == nullptr)
    return indices;

  for (int i = 0; i < mesh->getNodeCount(); ++i) {
    Node* node = mesh->getNode(i);
    if (node == nullptr)
      continue;

    const Vector3f& pos = node->getPos();
    if (wfgui::workflow::point_in_box(pos.x, pos.y, pos.z,
                                      xmin, ymin, zmin,
                                      xmax, ymax, zmax)) {
      indices.push_back(i);
    }
  }

  return indices;
}

inline std::vector<int> find_node_ids_in_box(Mesh* mesh,
                                             double xmin,
                                             double ymin,
                                             double zmin,
                                             double xmax,
                                             double ymax,
                                             double zmax)
{
  std::vector<int> ids;
  if (mesh == nullptr)
    return ids;

  for (int i = 0; i < mesh->getNodeCount(); ++i) {
    Node* node = mesh->getNode(i);
    if (node == nullptr)
      continue;

    const Vector3f& pos = node->getPos();
    if (wfgui::workflow::point_in_box(pos.x, pos.y, pos.z,
                                      xmin, ymin, zmin,
                                      xmax, ymax, zmax)) {
      ids.push_back(node->getId());
    }
  }

  return ids;
}

inline int add_node_set_from_box(Mesh* mesh,
                                 const std::string& name,
                                 double xmin,
                                 double ymin,
                                 double zmin,
                                 double xmax,
                                 double ymax,
                                 double zmax,
                                 int set_id = -1)
{
  const std::vector<int> node_indices =
    find_node_indices_in_box(mesh, xmin, ymin, zmin, xmax, ymax, zmax);
  return add_node_set_from_indices(mesh, name, node_indices, set_id);
}

#endif
