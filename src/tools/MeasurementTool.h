#ifndef MEASUREMENT_TOOL_H
#define MEASUREMENT_TOOL_H

#include <array>
#include <string>
#include <vector>

#include <vtkSmartPointer.h>
#include <vtkDataSet.h>
#include <vtkProp.h>

class Model;
class Mesh;
class Node;
class VtkViewer;

class MeasurementTool {
public:
  MeasurementTool() = default;
  ~MeasurementTool();

  void setContext(VtkViewer* viewer,
                  Model* model,
                  Mesh* targetMesh = nullptr,
                  vtkDataSet* targetDataSet = nullptr);
  void setEnabled(bool enabled);
  bool isEnabled() const { return m_enabled; }

  void handleInteraction();
  void drawOverlay() const;
  void clear();

private:
  struct MeasurementPoint {
    Node* node = nullptr;
    vtkIdType pointId = -1;
    bool isGlobalOrigin = false;
    std::array<double, 3> world = {0.0, 0.0, 0.0};
  };

  bool projectWorldToViewport(const std::array<double, 3>& world, double& x, double& y) const;
  bool projectNodeToViewport(Node* node, double& x, double& y) const;
  Node* pickClosestNodeAt(double x, double y, double maxDistancePixels = 12.0) const;
  bool tryPickGlobalOrigin(double x, double y, MeasurementPoint& outPoint) const;
  bool tryPickPoint(double x, double y, MeasurementPoint& outPoint) const;
  void rebuildOverlay();
  void clearOverlay();
  std::string buildMeasurementLabel() const;

private:
  VtkViewer* m_viewer = nullptr;
  Model* m_model = nullptr;
  Mesh* m_target_mesh = nullptr;
  vtkDataSet* m_target_data_set = nullptr;
  bool m_enabled = false;
  std::vector<MeasurementPoint> m_points;
  std::vector<vtkSmartPointer<vtkProp>> m_overlayActors;
};

#endif
