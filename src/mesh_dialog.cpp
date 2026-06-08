#include "mesh_dialog.h"
#include "geom/Geom.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <gmsh.h>

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <map>
#include <sstream>
#include <vector>


using namespace std;
namespace fs = std::filesystem;

namespace {

int curveNodeCountFromElementSizeForPreview(int curveTag, double elementSize)
{
  if (elementSize <= 0.0) {
    return 2;
  }

  double xmin = 0.0;
  double ymin = 0.0;
  double zmin = 0.0;
  double xmax = 0.0;
  double ymax = 0.0;
  double zmax = 0.0;
  gmsh::model::getBoundingBox(1, curveTag, xmin, ymin, zmin, xmax, ymax, zmax);

  const double dx = xmax - xmin;
  const double dy = ymax - ymin;
  const double dz = zmax - zmin;
  const double approxLength = std::sqrt(dx * dx + dy * dy + dz * dz);

  return std::max(2, static_cast<int>(std::ceil(approxLength / elementSize)) + 1);
}

double curveApproxLengthForPreview(int curveTag)
{
  double xmin = 0.0;
  double ymin = 0.0;
  double zmin = 0.0;
  double xmax = 0.0;
  double ymax = 0.0;
  double zmax = 0.0;
  gmsh::model::getBoundingBox(1, curveTag, xmin, ymin, zmin, xmax, ymax, zmax);

  const double dx = xmax - xmin;
  const double dy = ymax - ymin;
  const double dz = zmax - zmin;
  return std::sqrt(dx * dx + dy * dy + dz * dz);
}

void curveCenterForPreview(int curveTag, double &x, double &y, double &z)
{
  double xmin = 0.0;
  double ymin = 0.0;
  double zmin = 0.0;
  double xmax = 0.0;
  double ymax = 0.0;
  double zmax = 0.0;
  gmsh::model::getBoundingBox(1, curveTag, xmin, ymin, zmin, xmax, ymax, zmax);

  x = 0.5 * (xmin + xmax);
  y = 0.5 * (ymin + ymax);
  z = 0.5 * (zmin + zmax);
}

void surfaceCenterForPreview(int surfaceTag, double &x, double &y, double &z)
{
  double xmin = 0.0;
  double ymin = 0.0;
  double zmin = 0.0;
  double xmax = 0.0;
  double ymax = 0.0;
  double zmax = 0.0;
  gmsh::model::getBoundingBox(2, surfaceTag, xmin, ymin, zmin, xmax, ymax, zmax);

  x = 0.5 * (xmin + xmax);
  y = 0.5 * (ymin + ymax);
  z = 0.5 * (zmin + zmax);
}

std::string buildPreviewModelName(const Part *part)
{
  std::ostringstream stream;
  stream << "__mesh_preview_part_" << reinterpret_cast<std::uintptr_t>(part);
  return stream.str();
}

bool buildCurveDivisionPreview(Model *model,
                               Part *part,
                               double elementSize,
                               std::vector<MeshDialog::CurveDivisionPreview> &preview,
                               std::string &status)
{
  preview.clear();
  status.clear();

  if (model == nullptr || part == nullptr || part->getGeom() == nullptr) {
    status = "No geometry available for preview.";
    return false;
  }

  if (elementSize <= 0.0) {
    status = "Element size must be greater than zero.";
    return false;
  }

  std::string previousModelName;
  bool hadPreviousModel = false;
  try {
    gmsh::model::getCurrent(previousModelName);
    hadPreviousModel = !previousModelName.empty();
  } catch (...) {
    hadPreviousModel = false;
  }

  const std::string previewModelName = buildPreviewModelName(part);
  const fs::path previewPath =
    fs::temp_directory_path() / (previewModelName + ".step");
  const std::string originalGeomFileName = part->getGeom()->getName();
  bool previewModelCreated = false;

  try {
    part->getGeom()->setFileName(previewPath.string());
    part->getGeom()->ExportSTEP();

    gmsh::model::add(previewModelName);
    previewModelCreated = true;
    std::vector<std::pair<int, int>> importedEntities;
    gmsh::model::occ::importShapes(previewPath.string(), importedEntities);
    gmsh::vectorpair healedEntities;
    gmsh::model::occ::healShapes(healedEntities, importedEntities);
    gmsh::model::occ::removeAllDuplicates();
    gmsh::model::occ::synchronize();

    std::vector<std::pair<int, int>> curveEntities;
    gmsh::model::getEntities(curveEntities, 1);
    preview.reserve(curveEntities.size());

    for (const auto &entity : curveEntities) {
      MeshDialog::CurveDivisionPreview item;
      item.tag = entity.second;
      item.approx_length = curveApproxLengthForPreview(entity.second);
      item.node_count = curveNodeCountFromElementSizeForPreview(entity.second, elementSize);
      item.segment_count = std::max(1, item.node_count - 1);
      curveCenterForPreview(entity.second, item.center_x, item.center_y, item.center_z);
      preview.push_back(item);
    }

    std::sort(preview.begin(), preview.end(),
              [](const MeshDialog::CurveDivisionPreview &a,
                 const MeshDialog::CurveDivisionPreview &b) {
                return a.tag < b.tag;
              });

    status = preview.empty()
      ? "No curve entities found in geometry."
      : "Preview generated from the current geometry and target element size.";
  } catch (const std::exception &e) {
    status = std::string("Preview failed: ") + e.what();
  } catch (...) {
    status = "Preview failed due to an unknown error.";
  }

  part->getGeom()->setFileName(originalGeomFileName);

  if (previewModelCreated) {
    try {
      gmsh::model::setCurrent(previewModelName);
      gmsh::model::remove();
    } catch (...) {
    }
  }

  if (hadPreviousModel) {
    try {
      gmsh::model::setCurrent(previousModelName);
    } catch (...) {
    }
  }

  std::error_code removeError;
  fs::remove(previewPath, removeError);

  return !preview.empty();
}

} // namespace

void MeshDialog::Draw(const char* title, bool* p_open, Model *model, Part *part){
  
  
  if (!m_initialized) {
      m_id = part->getId();
      m_initialized = true;
      //m_v = part->getVel();
      
      if (model != nullptr)
        m_element_size = model->getElementSize();
      else
        m_element_size = 1.0f;
      if (model != nullptr)
        m_2d_mesh_generator = static_cast<int>(model->getTwoDMeshGenerator());
      else
        m_2d_mesh_generator = 0;
      m_apply_transfinite_surfaces = true;
      m_curve_preview_visible = false;
      m_curve_preview_dirty = false;
      m_seed_pick_mode = false;
      m_selected_curve_tag = -1;
      m_curve_preview_status.clear();
      m_curve_preview.clear();
      m_surface_preview.clear();
      
      if (part->getType() == Elastic)
        part_type = 0;
      else 
        part_type = 1;
  }
  
  create_part = false; 
  
  
  
  if (!ImGui::Begin(title, p_open)) {
      ImGui::End();
      return;
  }



  ImGui::InputInt("Id ", &m_id, 1, 10);  

  // Radio buttons para tipo
  if (ImGui::RadioButton("Deformable", part_type == 0)) {
      part_type = 0;
  }
  ImGui::SameLine();
  if (ImGui::RadioButton("Rigid", part_type == 1)) {
      part_type = 1;
  }

  // Velocidad
  float vv[3] = {static_cast<float>(m_v.x),
                 static_cast<float>(m_v.y),
                 static_cast<float>(m_v.z)};
  ImGui::InputFloat3("Velocity", vv, "%.4f");
  m_v.x = vv[0];
  m_v.y = vv[1];
  m_v.z = vv[2];
  
  // NUEVO: Deslizador para tamaño de elemento
  ImGui::Spacing();
  ImGui::Separator();
  ImGui::Text("Mesh Settings");
  
  // Define un rango razonable para el tamaño de elemento
  float min_size = 0.01f;
  float max_size = 10.0f;
  
  bool preview_settings_changed = false;

  preview_settings_changed |=
    ImGui::SliderFloat("Element Size", &m_element_size, min_size, max_size, "%.2e", ImGuiSliderFlags_Logarithmic);
  ImGui::SameLine();
  //HelpMarker("Smaller values = finer mesh, larger values = coarser mesh");
  
  // Input numérico para mayor precisión
  preview_settings_changed |=
    ImGui::InputFloat("##ElementSizeInput", &m_element_size, 1.0e-3f, 1.0e-2f, "%.2e");
  ImGui::SameLine();
  if (ImGui::Button("Default##Size")) {
      m_element_size = 1.0f; // Valor por defecto
      preview_settings_changed = true;
  }

  if (model != nullptr && model->getDimension() == 2 && part->getType() == Elastic) {
      const char* mesher_items[] = {"mesh-adapt", "gmsh"};
      preview_settings_changed |=
        ImGui::Combo("2D mesher", &m_2d_mesh_generator, mesher_items, IM_ARRAYSIZE(mesher_items));
  }

  if (preview_settings_changed && m_curve_preview_visible) {
    m_curve_preview_dirty = true;
  }

  ImGui::Spacing();
  if (ImGui::Button("Preview line divisions")) {
    std::map<int, std::pair<bool, int>> customSegmentsByTag;
    for (const CurveDivisionPreview &entry : m_curve_preview) {
      customSegmentsByTag[entry.tag] = {entry.use_custom_segments, entry.custom_segment_count};
    }
    m_curve_preview_visible = true;
    m_curve_preview_dirty = false;
    buildCurveDivisionPreview(model, part, m_element_size, m_curve_preview, m_curve_preview_status);
    m_surface_preview.clear();
    if (model != nullptr && model->getDimension() == 2) {
      std::string previousModelName;
      bool hadPreviousModel = false;
      try {
        gmsh::model::getCurrent(previousModelName);
        hadPreviousModel = !previousModelName.empty();
      } catch (...) {
        hadPreviousModel = false;
      }

      const std::string previewModelName = buildPreviewModelName(part) + "_surfaces";
      const fs::path previewPath = fs::temp_directory_path() / (previewModelName + ".step");
      const std::string originalGeomFileName = part->getGeom()->getName();
      bool previewModelCreated = false;
      try {
        part->getGeom()->setFileName(previewPath.string());
        part->getGeom()->ExportSTEP();
        gmsh::model::add(previewModelName);
        previewModelCreated = true;
        std::vector<std::pair<int, int>> importedEntities;
        gmsh::model::occ::importShapes(previewPath.string(), importedEntities);
        gmsh::vectorpair healedEntities;
        gmsh::model::occ::healShapes(healedEntities, importedEntities);
        gmsh::model::occ::removeAllDuplicates();
        gmsh::model::occ::synchronize();
        std::vector<std::pair<int, int>> surfaceEntities;
        gmsh::model::getEntities(surfaceEntities, 2);
        for (const auto &entity : surfaceEntities) {
          SurfacePreview item;
          item.tag = entity.second;
          surfaceCenterForPreview(entity.second, item.center_x, item.center_y, item.center_z);
          m_surface_preview.push_back(item);
        }
      } catch (...) {
      }
      part->getGeom()->setFileName(originalGeomFileName);
      if (previewModelCreated) {
        try {
          gmsh::model::setCurrent(previewModelName);
          gmsh::model::remove();
        } catch (...) {
        }
      }
      if (hadPreviousModel) {
        try {
          gmsh::model::setCurrent(previousModelName);
        } catch (...) {
        }
      }
      std::error_code removeError;
      fs::remove(previewPath, removeError);
    }
    for (CurveDivisionPreview &entry : m_curve_preview) {
      std::map<int, std::pair<bool, int>>::const_iterator existing = customSegmentsByTag.find(entry.tag);
      if (existing != customSegmentsByTag.end()) {
        entry.use_custom_segments = existing->second.first;
        entry.custom_segment_count = existing->second.second;
        if (entry.use_custom_segments && entry.custom_segment_count < 1) {
          entry.use_custom_segments = false;
          entry.custom_segment_count = 0;
        }
      }
    }
    if (m_selected_curve_tag >= 0) {
      bool curveStillExists = false;
      for (const CurveDivisionPreview &entry : m_curve_preview) {
        if (entry.tag == m_selected_curve_tag) {
          curveStillExists = true;
          break;
        }
      }
      if (!curveStillExists) {
        m_selected_curve_tag = -1;
      }
    }
  }

  if (m_curve_preview_visible) {
    const bool usesMeshAdapt =
      model != nullptr &&
      model->getDimension() == 2 &&
      part->getType() == Elastic &&
      m_2d_mesh_generator == static_cast<int>(MeshGeneratorMeshAdapt);

    if (m_curve_preview_dirty) {
      ImGui::TextDisabled("Preview out of date. Click again to refresh.");
    }

    if (usesMeshAdapt) {
      ImGui::TextWrapped("mesh-adapt does not expose the exact final subdivisions in advance. "
                         "This preview shows a base estimate from the current geometry and target element size.");
    } else {
      ImGui::TextWrapped("This preview matches the current geometry and target element size used to seed line divisions before meshing.");
    }

    if (!m_curve_preview_status.empty()) {
      ImGui::TextWrapped("%s", m_curve_preview_status.c_str());
    }

    if (model != nullptr && model->getDimension() == 2 && !usesMeshAdapt) {
      ImGui::Checkbox("Apply transfinite surfaces", &m_apply_transfinite_surfaces);
      if (!m_surface_preview.empty()) {
        ImGui::TextDisabled("%d surfaces detected for transfinite marking.", static_cast<int>(m_surface_preview.size()));
      }
    }

    if (usesMeshAdapt) {
      m_seed_pick_mode = false;
      ImGui::BeginDisabled();
      ImGui::Button("Seed by click");
      ImGui::EndDisabled();
      ImGui::SameLine();
      ImGui::TextDisabled("Available only with gmsh mesher.");
    } else if (ImGui::Button(m_seed_pick_mode ? "Stop seed by click" : "Seed by click")) {
      m_seed_pick_mode = !m_seed_pick_mode;
    }
    if (m_seed_pick_mode) {
      ImGui::SameLine();
      ImGui::TextDisabled("Click a curve in the viewport.");
    }

    const CurveDivisionPreview* selectedCurve = usesMeshAdapt ? nullptr : getSelectedCurvePreview();
    if (selectedCurve != nullptr) {
      ImGui::Separator();
      ImGui::Text("Selected curve: %d", selectedCurve->tag);
      ImGui::Text("Current segments: %d", selectedCurve->use_custom_segments
                                             ? selectedCurve->custom_segment_count
                                             : selectedCurve->segment_count);
      int editableSegments = selectedCurve->use_custom_segments
        ? selectedCurve->custom_segment_count
        : selectedCurve->segment_count;
      if (ImGui::InputInt("Segments", &editableSegments, 1, 5)) {
        if (editableSegments < 1) {
          editableSegments = 1;
        }
        setSelectedCurveSegments(editableSegments);
      }
      if (ImGui::Button("Reset selected seed")) {
        resetSelectedCurveSegments();
      }
      ImGui::SameLine();
      if (ImGui::Button("Reset all seeds")) {
        resetAllCurveSegments();
      }
    }

    if (!m_curve_preview.empty() &&
        ImGui::BeginTable("curve_division_preview", 4,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchSame)) {
      ImGui::TableSetupColumn("Curve");
      ImGui::TableSetupColumn("Approx length");
      ImGui::TableSetupColumn("Nodes");
      ImGui::TableSetupColumn("Segments");
      ImGui::TableHeadersRow();

      for (const CurveDivisionPreview &entry : m_curve_preview) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::Text("%d", entry.tag);
      ImGui::TableSetColumnIndex(1);
      ImGui::Text("%.6g", entry.approx_length);
      ImGui::TableSetColumnIndex(2);
      ImGui::Text("%d", entry.use_custom_segments ? (entry.custom_segment_count + 1) : entry.node_count);
      ImGui::TableSetColumnIndex(3);
      ImGui::Text("%d", entry.use_custom_segments ? entry.custom_segment_count : entry.segment_count);
      }

      ImGui::EndTable();
    }
  }

  // Botones Ok/Cancel
  ImGui::Spacing();
  ImGui::Separator();
  
  if (ImGui::Button("Ok")) {
    part->setId(m_id);
    part->setVel(m_v);
    part->setType(part_type);
    if (model != nullptr)
      model->setElementSize(m_element_size);
    if (model != nullptr)
      model->setTwoDMeshGenerator(static_cast<TwoDMeshGenerator>(m_2d_mesh_generator));
    m_apply_mesh = true;
    m_mesh_part = part;
    
    m_initialized = false;
    m_seed_pick_mode = false;
    *p_open = false;
  }
  
  ImGui::SameLine();
  if (ImGui::Button("Cancel")) {
    m_apply_mesh = false;
    m_mesh_part = nullptr;
    m_initialized = false;
    m_seed_pick_mode = false;
    *p_open = false;
  }
  
  ImGui::End();
}

const MeshDialog::CurveDivisionPreview* MeshDialog::getSelectedCurvePreview() const
{
  for (const CurveDivisionPreview &entry : m_curve_preview) {
    if (entry.tag == m_selected_curve_tag) {
      return &entry;
    }
  }
  return nullptr;
}

bool MeshDialog::selectCurveByTag(int tag)
{
  for (const CurveDivisionPreview &entry : m_curve_preview) {
    if (entry.tag == tag) {
      m_selected_curve_tag = tag;
      return true;
    }
  }
  return false;
}

bool MeshDialog::setSelectedCurveSegments(int segmentCount)
{
  if (segmentCount < 1) {
    segmentCount = 1;
  }

  for (CurveDivisionPreview &entry : m_curve_preview) {
    if (entry.tag == m_selected_curve_tag) {
      entry.use_custom_segments = true;
      entry.custom_segment_count = segmentCount;
      return true;
    }
  }
  return false;
}

void MeshDialog::resetSelectedCurveSegments()
{
  for (CurveDivisionPreview &entry : m_curve_preview) {
    if (entry.tag == m_selected_curve_tag) {
      entry.use_custom_segments = false;
      entry.custom_segment_count = 0;
      return;
    }
  }
}

void MeshDialog::resetAllCurveSegments()
{
  for (CurveDivisionPreview &entry : m_curve_preview) {
    entry.use_custom_segments = false;
    entry.custom_segment_count = 0;
  }
}

int MeshDialog::getCurveNodeCountOverride(int tag) const
{
  for (const CurveDivisionPreview &entry : m_curve_preview) {
    if (entry.tag == tag && entry.use_custom_segments && entry.custom_segment_count > 0) {
      return entry.custom_segment_count + 1;
    }
  }
  return 0;
}
