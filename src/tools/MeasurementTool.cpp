#include "tools/MeasurementTool.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "imgui.h"

#include "model/Model.h"
#include "model/Mesh.h"
#include "Node.h"
#include "Part.h"
#include "VtkViewer.h"

#include <vtkActor.h>
#include <vtkDataSet.h>
#include <vtkLineSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>

MeasurementTool::~MeasurementTool()
{
  clearOverlay();
}

void MeasurementTool::setContext(VtkViewer* viewer,
                                 Model* model,
                                 Mesh* targetMesh,
                                 vtkDataSet* targetDataSet)
{
  if (m_viewer == viewer &&
      m_model == model &&
      m_target_mesh == targetMesh &&
      m_target_data_set == targetDataSet) {
    return;
  }

  clearOverlay();
  m_viewer = viewer;
  m_model = model;
  m_target_mesh = targetMesh;
  m_target_data_set = targetDataSet;
}

void MeasurementTool::setEnabled(bool enabled)
{
  if (m_enabled == enabled) {
    return;
  }

  m_enabled = enabled;
  if (!m_enabled) {
    clear();
  }
}

void MeasurementTool::handleInteraction()
{
  if (!m_enabled || m_viewer == nullptr || m_model == nullptr) {
    return;
  }

  const ImVec2 viewportMin = m_viewer->getViewportScreenMin();
  const ImVec2 viewportMax = m_viewer->getViewportScreenMax();
  if (viewportMax.x <= viewportMin.x || viewportMax.y <= viewportMin.y) {
    return;
  }

  if (!m_viewer->isViewportHovered() || !ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
    return;
  }

  ImGuiIO& io = ImGui::GetIO();
  const ImVec2 localMouse(io.MousePos.x - viewportMin.x, io.MousePos.y - viewportMin.y);

  MeasurementPoint pickedPoint;
  if (!tryPickPoint(localMouse.x, localMouse.y, pickedPoint)) {
    return;
  }

  if (m_points.size() >= 2) {
    m_points.clear();
  }

  m_points.push_back(pickedPoint);
  rebuildOverlay();
}

void MeasurementTool::drawOverlay() const
{
  if (!m_enabled || m_viewer == nullptr) {
    return;
  }

  const ImVec2 viewportMin = m_viewer->getViewportScreenMin();
  const ImVec2 viewportMax = m_viewer->getViewportScreenMax();
  if (viewportMax.x <= viewportMin.x || viewportMax.y <= viewportMin.y) {
    return;
  }

  ImDrawList* drawList = ImGui::GetForegroundDrawList();
  for (std::size_t i = 0; i < m_points.size(); ++i) {
    double x = 0.0;
    double y = 0.0;
    if (!projectWorldToViewport(m_points[i].world, x, y)) {
      continue;
    }

    const ImVec2 center(viewportMin.x + static_cast<float>(x),
                        viewportMin.y + static_cast<float>(y));
    drawList->AddCircleFilled(center, 5.5f, IM_COL32(255, 180, 0, 255));
    drawList->AddCircle(center, 9.0f, IM_COL32(255, 220, 80, 255), 0, 2.0f);

    char tag[8];
    if (m_points[i].isGlobalOrigin) {
      std::snprintf(tag, sizeof(tag), "O");
    } else {
      std::snprintf(tag, sizeof(tag), "%c", static_cast<char>('A' + i));
    }
    drawList->AddText(ImVec2(center.x + 10.0f, center.y - 16.0f),
                      IM_COL32(255, 230, 140, 255), tag);
  }

  if (m_points.size() == 1) {
    double x = 0.0;
    double y = 0.0;
    if (!projectWorldToViewport(m_points.front().world, x, y)) {
      return;
    }

    drawList->AddText(ImVec2(viewportMin.x + static_cast<float>(x) + 14.0f,
                             viewportMin.y + static_cast<float>(y) + 8.0f),
                      IM_COL32(255, 230, 140, 255),
                      "Pick point B");
    return;
  }

  if (m_points.size() != 2) {
    return;
  }

  double ax = 0.0;
  double ay = 0.0;
  double bx = 0.0;
  double by = 0.0;
  if (!projectWorldToViewport(m_points[0].world, ax, ay) ||
      !projectWorldToViewport(m_points[1].world, bx, by)) {
    return;
  }

  const ImVec2 a(viewportMin.x + static_cast<float>(ax), viewportMin.y + static_cast<float>(ay));
  const ImVec2 b(viewportMin.x + static_cast<float>(bx), viewportMin.y + static_cast<float>(by));
  drawList->AddLine(a, b, IM_COL32(255, 180, 0, 220), 2.0f);

  const ImVec2 mid((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f);
  const std::string label = buildMeasurementLabel();
  const ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
  const ImVec2 textPos(mid.x + 12.0f, mid.y - textSize.y - 12.0f);
  drawList->AddRectFilled(ImVec2(textPos.x - 6.0f, textPos.y - 4.0f),
                          ImVec2(textPos.x + textSize.x + 6.0f, textPos.y + textSize.y + 4.0f),
                          IM_COL32(0, 0, 0, 150), 4.0f);
  drawList->AddText(textPos, IM_COL32(255, 230, 140, 255), label.c_str());
}

void MeasurementTool::clear()
{
  m_points.clear();
  clearOverlay();
}

bool MeasurementTool::projectWorldToViewport(const std::array<double, 3>& world,
                                             double& x,
                                             double& y) const
{
  if (m_viewer == nullptr || m_viewer->getRenderer() == nullptr) {
    return false;
  }

  vtkRenderer* renderer = m_viewer->getRenderer();
  renderer->SetWorldPoint(world[0], world[1], world[2], 1.0);
  renderer->WorldToDisplay();

  double display[3] = {0.0, 0.0, 0.0};
  renderer->GetDisplayPoint(display);
  if (display[2] < 0.0 || display[2] > 1.0) {
    return false;
  }

  x = display[0];
  y = static_cast<double>(m_viewer->getViewportHeight()) - display[1];
  return true;
}

bool MeasurementTool::projectNodeToViewport(Node* node, double& x, double& y) const
{
  if (node == nullptr) {
    return false;
  }

  const Vector3f& pos = node->getPos();
  return projectWorldToViewport({pos.x, pos.y, pos.z}, x, y);
}

bool MeasurementTool::tryPickGlobalOrigin(double x, double y, MeasurementPoint& outPoint) const
{
  if (m_viewer == nullptr || !m_viewer->isGlobalOriginVisible()) {
    return false;
  }

  double sx = 0.0;
  double sy = 0.0;
  if (!projectWorldToViewport(m_viewer->getGlobalOriginWorldPoint(), sx, sy)) {
    return false;
  }

  const double dx = sx - x;
  const double dy = sy - y;
  const double dist2 = dx * dx + dy * dy;
  if (dist2 > 14.0 * 14.0) {
    return false;
  }

  outPoint.node = nullptr;
  outPoint.pointId = -1;
  outPoint.isGlobalOrigin = true;
  outPoint.world = m_viewer->getGlobalOriginWorldPoint();
  return true;
}

Node* MeasurementTool::pickClosestNodeAt(double x, double y, double maxDistancePixels) const
{
  if (m_model == nullptr) {
    return nullptr;
  }

  Node* closest = nullptr;
  double bestDist2 = maxDistancePixels * maxDistancePixels;

  if (m_target_data_set != nullptr) {
    for (vtkIdType pointId = 0; pointId < m_target_data_set->GetNumberOfPoints(); ++pointId) {
      double worldPoint[3] = {0.0, 0.0, 0.0};
      m_target_data_set->GetPoint(pointId, worldPoint);

      double sx = 0.0;
      double sy = 0.0;
      if (!projectWorldToViewport({worldPoint[0], worldPoint[1], worldPoint[2]}, sx, sy)) {
        continue;
      }

      const double dx = sx - x;
      const double dy = sy - y;
      const double dist2 = dx * dx + dy * dy;
      if (dist2 <= bestDist2) {
        bestDist2 = dist2;
        closest = nullptr;
      }
    }
    return closest;
  }

  auto inspectMesh = [&](Mesh* mesh) {
    if (mesh == nullptr) {
      return;
    }

    for (int n = 0; n < mesh->getNodeCount(); ++n) {
      Node* node = mesh->getNode(n);
      double sx = 0.0;
      double sy = 0.0;
      if (!projectNodeToViewport(node, sx, sy)) {
        continue;
      }

      const double dx = sx - x;
      const double dy = sy - y;
      const double dist2 = dx * dx + dy * dy;
      if (dist2 <= bestDist2) {
        bestDist2 = dist2;
        closest = node;
      }
    }
  };

  if (m_target_mesh != nullptr) {
    inspectMesh(m_target_mesh);
    return closest;
  }

  for (int p = 0; p < m_model->getPartCount(); ++p) {
    Part* part = m_model->getPart(p);
    if (part == nullptr || !part->isMeshed() || part->getMesh() == nullptr) {
      continue;
    }

    inspectMesh(part->getMesh());
  }

  return closest;
}

bool MeasurementTool::tryPickPoint(double x, double y, MeasurementPoint& outPoint) const
{
  if (tryPickGlobalOrigin(x, y, outPoint)) {
    return true;
  }

  if (m_target_data_set != nullptr) {
    vtkIdType closestPointId = -1;
    double bestDist2 = 12.0 * 12.0;

    for (vtkIdType pointId = 0; pointId < m_target_data_set->GetNumberOfPoints(); ++pointId) {
      double worldPoint[3] = {0.0, 0.0, 0.0};
      m_target_data_set->GetPoint(pointId, worldPoint);

      double sx = 0.0;
      double sy = 0.0;
      if (!projectWorldToViewport({worldPoint[0], worldPoint[1], worldPoint[2]}, sx, sy)) {
        continue;
      }

      const double dx = sx - x;
      const double dy = sy - y;
      const double dist2 = dx * dx + dy * dy;
      if (dist2 <= bestDist2) {
        bestDist2 = dist2;
        closestPointId = pointId;
        outPoint.world = {worldPoint[0], worldPoint[1], worldPoint[2]};
      }
    }

    if (closestPointId < 0) {
      return false;
    }

    outPoint.node = nullptr;
    outPoint.pointId = closestPointId;
    outPoint.isGlobalOrigin = false;
    return true;
  }

  Node* node = pickClosestNodeAt(x, y);
  if (node == nullptr) {
    return false;
  }

  const Vector3f& pos = node->getPos();
  outPoint.node = node;
  outPoint.pointId = -1;
  outPoint.isGlobalOrigin = false;
  outPoint.world = {pos.x, pos.y, pos.z};
  return true;
}

void MeasurementTool::rebuildOverlay()
{
  clearOverlay();

  if (m_viewer == nullptr || m_viewer->getRenderer() == nullptr || m_points.size() != 2) {
    return;
  }

  vtkSmartPointer<vtkLineSource> lineSource = vtkSmartPointer<vtkLineSource>::New();
  lineSource->SetPoint1(m_points[0].world[0], m_points[0].world[1], m_points[0].world[2]);
  lineSource->SetPoint2(m_points[1].world[0], m_points[1].world[1], m_points[1].world[2]);

  vtkSmartPointer<vtkPolyDataMapper> lineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  lineMapper->SetInputConnection(lineSource->GetOutputPort());

  vtkSmartPointer<vtkActor> lineActor = vtkSmartPointer<vtkActor>::New();
  lineActor->SetMapper(lineMapper);
  lineActor->PickableOff();
  lineActor->GetProperty()->SetColor(1.0, 0.7, 0.1);
  lineActor->GetProperty()->SetLineWidth(3.0);
  lineActor->GetProperty()->LightingOff();
  lineActor->GetProperty()->SetRenderLinesAsTubes(true);

  m_viewer->getRenderer()->AddActor(lineActor);
  m_overlayActors.push_back(lineActor);
}

void MeasurementTool::clearOverlay()
{
  if (m_viewer == nullptr || m_viewer->getRenderer() == nullptr) {
    m_overlayActors.clear();
    return;
  }

  vtkRenderer* renderer = m_viewer->getRenderer();
  for (const vtkSmartPointer<vtkProp>& actor : m_overlayActors) {
    if (actor != nullptr && renderer->HasViewProp(actor)) {
      renderer->RemoveViewProp(actor);
    }
  }
  m_overlayActors.clear();
}

std::string MeasurementTool::buildMeasurementLabel() const
{
  if (m_points.size() != 2) {
    return "";
  }

  const double dx = m_points[1].world[0] - m_points[0].world[0];
  const double dy = m_points[1].world[1] - m_points[0].world[1];
  const double dz = m_points[1].world[2] - m_points[0].world[2];
  const double total = std::sqrt(dx * dx + dy * dy + dz * dz);

  char buffer[256];
  std::snprintf(buffer, sizeof(buffer),
                "dX=%.6g  dY=%.6g  dZ=%.6g  |d|=%.6g",
                dx, dy, dz, total);
  return std::string(buffer);
}
