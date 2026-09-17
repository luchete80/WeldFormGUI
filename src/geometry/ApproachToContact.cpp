#include "ApproachToContact.h"

#include <vtkCellData.h>
#include <vtkCell.h>
#include <vtkCleanPolyData.h>
#include <vtkDataArray.h>
#include <vtkGenericCell.h>
#include <vtkIdTypeArray.h>
#include <vtkNew.h>
#include <vtkPolyData.h>
#include <vtkSelectEnclosedPoints.h>
#include <vtkStaticCellLocator.h>
#include <vtkSmartPointer.h>
#include <vtkTriangleFilter.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <numeric>

namespace {

struct Vec3 {
  double x = 0.0, y = 0.0, z = 0.0;
  double& operator[](int i) { return i == 0 ? x : (i == 1 ? y : z); }
  double operator[](int i) const { return i == 0 ? x : (i == 1 ? y : z); }
};

Vec3 operator+(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
Vec3 operator-(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
Vec3 operator*(Vec3 a, double s) { return {a.x * s, a.y * s, a.z * s}; }
double dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
Vec3 cross(Vec3 a, Vec3 b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
double norm(Vec3 a) { return std::sqrt(dot(a, a)); }
bool finite(Vec3 a) { return std::isfinite(a.x) && std::isfinite(a.y) && std::isfinite(a.z); }

struct Bounds {
  double min[3] = {std::numeric_limits<double>::infinity(),
                   std::numeric_limits<double>::infinity(),
                   std::numeric_limits<double>::infinity()};
  double max[3] = {-std::numeric_limits<double>::infinity(),
                   -std::numeric_limits<double>::infinity(),
                   -std::numeric_limits<double>::infinity()};

  void add(Vec3 p) {
    for (int i = 0; i < 3; ++i) {
      min[i] = std::min(min[i], p[i]);
      max[i] = std::max(max[i], p[i]);
    }
  }
  void expand(double amount) {
    for (int i = 0; i < 3; ++i) { min[i] -= amount; max[i] += amount; }
  }
  bool valid() const { return min[0] <= max[0] && min[1] <= max[1] && min[2] <= max[2]; }
  double diagonal() const {
    if (!valid()) return 0.0;
    return std::sqrt((max[0] - min[0]) * (max[0] - min[0]) +
                     (max[1] - min[1]) * (max[1] - min[1]) +
                     (max[2] - min[2]) * (max[2] - min[2]));
  }
};

bool intersects(const Bounds& a, const Bounds& b) {
  for (int i = 0; i < 3; ++i)
    if (a.max[i] < b.min[i] || b.max[i] < a.min[i]) return false;
  return true;
}

struct Triangle {
  Vec3 p[3];
  vtkIdType originalCell = -1;
  Bounds bounds;
  Vec3 centroid;
};

struct TriangulatedSurface {
  std::vector<Triangle> triangles;
  Bounds bounds;
  vtkSmartPointer<vtkPolyData> data;
  std::string diagnostic;
};

bool triangulate(vtkPolyData* input, const char* label, TriangulatedSurface& output) {
  if (!input || input->GetNumberOfPoints() == 0 || input->GetNumberOfCells() == 0) {
    output.diagnostic = std::string(label) + " surface is empty";
    return false;
  }

  vtkNew<vtkPolyData> taggedInput;
  taggedInput->DeepCopy(input);
  vtkNew<vtkIdTypeArray> originalCellIds;
  originalCellIds->SetName("__approach_to_contact_original_cell_id");
  originalCellIds->SetNumberOfTuples(taggedInput->GetNumberOfCells());
  for (vtkIdType i = 0; i < taggedInput->GetNumberOfCells(); ++i)
    originalCellIds->SetValue(i, i);
  taggedInput->GetCellData()->AddArray(originalCellIds);

  // CAD and primitive sources often duplicate coincident points at face
  // boundaries. Welding those points on this private copy lets closure and
  // point-in-solid checks see the actual connected surface topology.
  vtkNew<vtkCleanPolyData> clean;
  clean->SetInputData(taggedInput);
  clean->ToleranceIsAbsoluteOn();
  clean->SetAbsoluteTolerance(0.0);
  clean->Update();
  const vtkIdType inputPolygonCount = taggedInput->GetNumberOfPolys();
  const vtkIdType cleanedPolygonCount = clean->GetOutput()->GetNumberOfPolys();

  vtkNew<vtkTriangleFilter> filter;
  filter->SetInputConnection(clean->GetOutputPort());
  filter->PassVertsOff();
  filter->PassLinesOff();
  filter->Update();
  output.data = filter->GetOutput();
  vtkIdTypeArray* sourceIds = vtkIdTypeArray::SafeDownCast(
    output.data->GetCellData()->GetArray(originalCellIds->GetName()));
  if (!sourceIds) {
    output.diagnostic = std::string(label) + " triangulation did not preserve source cell IDs";
    return false;
  }

  std::size_t skippedDegenerate = 0;
  for (vtkIdType cellId = 0; cellId < output.data->GetNumberOfCells(); ++cellId) {
    vtkCell* cell = output.data->GetCell(cellId);
    if (!cell || cell->GetNumberOfPoints() != 3) continue;
    Triangle tri;
    double p[3];
    for (int i = 0; i < 3; ++i) {
      output.data->GetPoint(cell->GetPointId(i), p);
      tri.p[i] = {p[0], p[1], p[2]};
      if (!finite(tri.p[i])) {
        output.diagnostic = std::string(label) + " surface contains a non-finite point";
        return false;
      }
      tri.bounds.add(tri.p[i]);
    }
    const Vec3 edge0 = tri.p[1] - tri.p[0];
    const Vec3 edge1 = tri.p[2] - tri.p[1];
    const Vec3 edge2 = tri.p[0] - tri.p[2];
    const double longestEdgeSquared = std::max({dot(edge0, edge0), dot(edge1, edge1), dot(edge2, edge2)});
    if (norm(cross(tri.p[1] - tri.p[0], tri.p[2] - tri.p[0])) <= longestEdgeSquared * 1e-14) {
      ++skippedDegenerate;
      continue;
    }
    tri.originalCell = sourceIds->GetValue(cellId);
    tri.centroid = (tri.p[0] + tri.p[1] + tri.p[2]) * (1.0 / 3.0);
    output.bounds.add(tri.p[0]);
    output.bounds.add(tri.p[1]);
    output.bounds.add(tri.p[2]);
    output.triangles.push_back(tri);
  }
  if (output.triangles.empty()) {
    output.diagnostic = std::string(label) + " surface has no non-degenerate triangles";
    return false;
  }
  if (cleanedPolygonCount < inputPolygonCount) {
    output.diagnostic = std::string(label) + " surface removed " +
      std::to_string(inputPolygonCount - cleanedPolygonCount) +
      " degenerate or duplicate polygon(s) during cleanup";
  }
  if (skippedDegenerate != 0) {
    if (!output.diagnostic.empty()) output.diagnostic += "; ";
    output.diagnostic += std::string(label) + " surface skipped " +
      std::to_string(skippedDegenerate) + " degenerate triangle(s)";
  }
  return true;
}

struct BvhNode {
  Bounds bounds;
  int left = -1;
  int right = -1;
  std::size_t begin = 0;
  std::size_t end = 0;
};

class TriangleBvh {
public:
  explicit TriangleBvh(const std::vector<Triangle>& triangles) : m_triangles(triangles) {
    m_order.resize(triangles.size());
    std::iota(m_order.begin(), m_order.end(), 0);
    if (!m_order.empty()) build(0, m_order.size());
  }

  void query(const Bounds& bounds, std::vector<std::size_t>& result) const {
    if (!m_nodes.empty()) queryNode(0, bounds, result);
  }

private:
  int build(std::size_t begin, std::size_t end) {
    const int nodeId = static_cast<int>(m_nodes.size());
    m_nodes.emplace_back();
    BvhNode& node = m_nodes.back();
    node.begin = begin;
    node.end = end;
    Bounds centroids;
    for (std::size_t i = begin; i < end; ++i) {
      const Triangle& t = m_triangles[m_order[i]];
      for (int a = 0; a < 3; ++a) {
        node.bounds.min[a] = std::min(node.bounds.min[a], t.bounds.min[a]);
        node.bounds.max[a] = std::max(node.bounds.max[a], t.bounds.max[a]);
      }
      centroids.add(t.centroid);
    }
    if (end - begin <= 8) return nodeId;

    int axis = 0;
    for (int a = 1; a < 3; ++a)
      if (centroids.max[a] - centroids.min[a] > centroids.max[axis] - centroids.min[axis]) axis = a;
    const std::size_t middle = begin + (end - begin) / 2;
    std::nth_element(m_order.begin() + begin, m_order.begin() + middle, m_order.begin() + end,
      [&](std::size_t lhs, std::size_t rhs) {
        return m_triangles[lhs].centroid[axis] < m_triangles[rhs].centroid[axis];
      });
    const int left = build(begin, middle);
    const int right = build(middle, end);
    m_nodes[nodeId].left = left;
    m_nodes[nodeId].right = right;
    return nodeId;
  }

  void queryNode(int nodeId, const Bounds& bounds, std::vector<std::size_t>& result) const {
    const BvhNode& node = m_nodes[nodeId];
    if (!intersects(node.bounds, bounds)) return;
    if (node.left < 0) {
      for (std::size_t i = node.begin; i < node.end; ++i) {
        const std::size_t triId = m_order[i];
        if (intersects(m_triangles[triId].bounds, bounds)) result.push_back(triId);
      }
      return;
    }
    queryNode(node.left, bounds, result);
    queryNode(node.right, bounds, result);
  }

  const std::vector<Triangle>& m_triangles;
  std::vector<std::size_t> m_order;
  std::vector<BvhNode> m_nodes;
};

Bounds sweptBounds(const Triangle& tri, Vec3 direction, double maxDistance, double tol) {
  Bounds b;
  for (Vec3 point : tri.p) {
    b.add(point);
    b.add(point + direction * maxDistance);
  }
  b.expand(tol);
  return b;
}

bool sweepTriangles(const Triangle& moving, const Triangle& target, Vec3 direction,
                    double tol, double maxDistance, double& enterDistance,
                    double& exitDistance) {
  Vec3 axes[11];
  axes[0] = cross(moving.p[1] - moving.p[0], moving.p[2] - moving.p[0]);
  axes[1] = cross(target.p[1] - target.p[0], target.p[2] - target.p[0]);
  Vec3 movingEdges[3] = {moving.p[1] - moving.p[0], moving.p[2] - moving.p[1], moving.p[0] - moving.p[2]};
  Vec3 targetEdges[3] = {target.p[1] - target.p[0], target.p[2] - target.p[1], target.p[0] - target.p[2]};
  int axisCount = 2;
  for (Vec3 me : movingEdges)
    for (Vec3 te : targetEdges)
      axes[axisCount++] = cross(me, te);

  double globalEnter = 0.0;
  double globalExit = maxDistance;
  for (int axisIndex = 0; axisIndex < axisCount; ++axisIndex) {
    Vec3 axis = axes[axisIndex];
    const double axisLength = norm(axis);
    if (axisLength <= std::numeric_limits<double>::min()) continue;
    axis = axis * (1.0 / axisLength);
    const Vec3 origin = target.p[0];
    double movingMin = dot(moving.p[0] - origin, axis), movingMax = movingMin;
    double targetMin = 0.0, targetMax = 0.0;
    for (int i = 1; i < 3; ++i) {
      const double mp = dot(moving.p[i] - origin, axis);
      const double tp = dot(target.p[i] - origin, axis);
      movingMin = std::min(movingMin, mp);
      movingMax = std::max(movingMax, mp);
      targetMin = std::min(targetMin, tp);
      targetMax = std::max(targetMax, tp);
    }
    const double speed = dot(direction, axis);
    if (std::abs(speed) <= 1e-14) {
      if (movingMax < targetMin - tol || targetMax < movingMin - tol) return false;
      continue;
    }

    double axisEnter, axisExit;
    if (speed > 0.0) {
      axisEnter = (targetMin - tol - movingMax) / speed;
      axisExit = (targetMax + tol - movingMin) / speed;
    } else {
      axisEnter = (targetMax + tol - movingMin) / speed;
      axisExit = (targetMin - tol - movingMax) / speed;
    }
    if (axisEnter > axisExit) std::swap(axisEnter, axisExit);
    globalEnter = std::max(globalEnter, axisEnter);
    globalExit = std::min(globalExit, axisExit);
    if (globalEnter > globalExit) return false;
  }

  if (globalExit < -tol || globalEnter > maxDistance + tol) return false;
  enterDistance = std::max(0.0, globalEnter);
  exitDistance = std::max(enterDistance, globalExit);
  return true;
}

class PointInsideQuery {
public:
  PointInsideQuery(vtkPolyData* surface, double tolerance) : m_tolerance(tolerance) {
    m_enclosed->SetTolerance(std::max(1e-12, tolerance * 0.1 / std::max(1e-150, [&]() {
      double bounds[6];
      surface->GetBounds(bounds);
      const double x = bounds[1] - bounds[0];
      const double y = bounds[3] - bounds[2];
      const double z = bounds[5] - bounds[4];
      return std::sqrt(x * x + y * y + z * z);
    }())));
    m_enclosed->Initialize(surface);
    m_locator->SetDataSet(surface);
    m_locator->BuildLocator();
  }

  bool isInside(Vec3 point) {
    return m_enclosed->IsInsideSurface(point.x, point.y, point.z) != 0;
  }

  bool isStrictlyInside(Vec3 point) {
    if (!isInside(point)) return false;
    double query[3] = {point.x, point.y, point.z};
    double closest[3];
    vtkIdType cellId = -1;
    int subId = -1;
    double distanceSquared = 0.0;
    m_locator->FindClosestPoint(query, closest, m_cell, cellId, subId, distanceSquared);
    return distanceSquared > m_tolerance * m_tolerance;
  }

private:
  double m_tolerance;
  vtkNew<vtkSelectEnclosedPoints> m_enclosed;
  vtkNew<vtkStaticCellLocator> m_locator;
  vtkNew<vtkGenericCell> m_cell;
};

bool pointInTriangle(Vec3 point, const Triangle& tri, double tolerance) {
  const Vec3 edge0 = tri.p[1] - tri.p[0];
  const Vec3 edge1 = tri.p[2] - tri.p[0];
  const Vec3 relative = point - tri.p[0];
  const double d00 = dot(edge0, edge0);
  const double d01 = dot(edge0, edge1);
  const double d11 = dot(edge1, edge1);
  const double d20 = dot(relative, edge0);
  const double d21 = dot(relative, edge1);
  const double denominator = d00 * d11 - d01 * d01;
  if (denominator <= std::numeric_limits<double>::min()) return false;
  const double v = (d11 * d20 - d01 * d21) / denominator;
  const double w = (d00 * d21 - d01 * d20) / denominator;
  const double barycentricTolerance = tolerance / std::max(1e-150, std::sqrt(std::max(d00, d11)));
  return v >= -barycentricTolerance && w >= -barycentricTolerance &&
         v + w <= 1.0 + barycentricTolerance;
}

void addSegmentTriangleIntersections(Vec3 p0, Vec3 p1, const Triangle& triangle,
                                     double tolerance, std::vector<Vec3>& intersections) {
  const Vec3 normal = cross(triangle.p[1] - triangle.p[0], triangle.p[2] - triangle.p[0]);
  const double d0 = dot(p0 - triangle.p[0], normal);
  const double d1 = dot(p1 - triangle.p[0], normal);
  const double planeTolerance = tolerance * norm(normal);
  if (std::abs(d0) <= planeTolerance && pointInTriangle(p0, triangle, tolerance))
    intersections.push_back(p0);
  if (std::abs(d1) <= planeTolerance && pointInTriangle(p1, triangle, tolerance))
    intersections.push_back(p1);
  if ((d0 < -planeTolerance && d1 < -planeTolerance) ||
      (d0 > planeTolerance && d1 > planeTolerance) || std::abs(d0 - d1) <= planeTolerance)
    return;
  const double parameter = d0 / (d0 - d1);
  if (parameter < 0.0 || parameter > 1.0) return;
  const Vec3 point = p0 + (p1 - p0) * std::clamp(parameter, 0.0, 1.0);
  if (pointInTriangle(point, triangle, tolerance)) intersections.push_back(point);
}

void triangleIntersections(const Triangle& a, const Triangle& b, double tolerance,
                           std::vector<Vec3>& intersections) {
  for (int i = 0; i < 3; ++i) {
    addSegmentTriangleIntersections(a.p[i], a.p[(i + 1) % 3], b, tolerance, intersections);
    addSegmentTriangleIntersections(b.p[i], b.p[(i + 1) % 3], a, tolerance, intersections);
  }
}

bool findInwardDirection(PointInsideQuery& query, const Triangle& triangle,
                         Vec3 point, double probe, Vec3& inward) {
  Vec3 normal = cross(triangle.p[1] - triangle.p[0], triangle.p[2] - triangle.p[0]);
  const double normalLength = norm(normal);
  if (normalLength <= std::numeric_limits<double>::min()) return false;
  normal = normal * (1.0 / normalLength);
  if (query.isStrictlyInside(point + normal * probe)) { inward = normal; return true; }
  if (query.isStrictlyInside(point - normal * probe)) { inward = normal * -1.0; return true; }
  return false;
}

bool hasInteriorOverlap(const TriangulatedSurface& a, const TriangulatedSurface& b,
                        double tolerance, const TriangleBvh& targetBvh,
                        bool aClosed, bool bClosed) {
  if (!aClosed && !bClosed) return false;
  std::unique_ptr<PointInsideQuery> insideA;
  std::unique_ptr<PointInsideQuery> insideB;
  if (aClosed) insideA = std::make_unique<PointInsideQuery>(a.data, tolerance);
  if (bClosed) insideB = std::make_unique<PointInsideQuery>(b.data, tolerance);
  auto anyInteriorSample = [](const TriangulatedSurface& samples, PointInsideQuery& query) {
    for (const Triangle& tri : samples.triangles) {
      if (query.isStrictlyInside(tri.centroid)) return true;
      for (Vec3 vertex : tri.p)
        if (query.isStrictlyInside(vertex)) return true;
    }
    return false;
  };
  if ((insideB && anyInteriorSample(a, *insideB)) ||
      (insideA && anyInteriorSample(b, *insideA))) return true;
  if (!aClosed || !bClosed) return false;

  // Vertices can all lie outside when two closed solids overlap only through
  // crossing faces. At each actual surface intersection, the sum of the two
  // inward normals points into the local common volume when one exists.
  const double probe = std::max(tolerance * 8.0,
    std::max(a.bounds.diagonal(), b.bounds.diagonal()) * 1e-10);
  std::vector<std::size_t> candidates;
  std::vector<Vec3> intersections;
  for (const Triangle& movingTri : a.triangles) {
    candidates.clear();
    Bounds queryBounds = movingTri.bounds;
    queryBounds.expand(tolerance);
    targetBvh.query(queryBounds, candidates);
    for (std::size_t targetId : candidates) {
      const Triangle& targetTri = b.triangles[targetId];
      if (!intersects(movingTri.bounds, targetTri.bounds)) continue;
      intersections.clear();
      triangleIntersections(movingTri, targetTri, tolerance, intersections);
      for (Vec3 intersection : intersections) {
        Vec3 inwardA, inwardB;
        if (!findInwardDirection(*insideA, movingTri, intersection, probe, inwardA) ||
            !findInwardDirection(*insideB, targetTri, intersection, probe, inwardB)) continue;
        const Vec3 commonDirection = inwardA + inwardB;
        const double commonLength = norm(commonDirection);
        if (commonLength <= 1e-10) continue;
        const Vec3 commonPoint = intersection + commonDirection * (probe / commonLength);
        if (insideA->isInside(commonPoint) && insideB->isInside(commonPoint)) return true;
      }
    }
  }
  return false;
}

void addUnique(std::vector<vtkIdType>& ids, vtkIdType id) {
  if (std::find(ids.begin(), ids.end(), id) == ids.end()) ids.push_back(id);
}

void appendDiagnostic(std::string& target, const std::string& value) {
  if (value.empty()) return;
  if (!target.empty()) target += "; ";
  target += value;
}

} // namespace

ApproachToContactResult computeApproachToContact(const ApproachToContactRequest& request) {
  ApproachToContactResult result;
  auto invalid = [&](const std::string& message) {
    result.status = ApproachStatus::InvalidInput;
    result.diagnostic = message;
    return result;
  };

  if (!request.movingSurface || !request.targetSurface)
    return invalid("moving and target surfaces are required");
  Vec3 direction{request.direction[0], request.direction[1], request.direction[2]};
  if (!finite(direction)) return invalid("direction must contain finite values");
  const double directionLength = norm(direction);
  if (!std::isfinite(directionLength) || directionLength <= 1e-14)
    return invalid("direction is zero, nearly zero, or too large to normalize");
  direction = direction * (1.0 / directionLength);
  if (!std::isfinite(request.targetGap) || request.targetGap < 0.0)
    return invalid("targetGap must be finite and non-negative");
  if (!std::isfinite(request.maximumMovement) || request.maximumMovement <= 0.0)
    return invalid("maximumMovement must be finite and greater than zero");
  if (!std::isfinite(request.tolerance) || request.tolerance < 0.0)
    return invalid("tolerance must be finite and non-negative");

  TriangulatedSurface moving, target;
  if (!triangulate(request.movingSurface, "moving", moving))
    return invalid(moving.diagnostic);
  if (!triangulate(request.targetSurface, "target", target))
    return invalid(target.diagnostic);

  const double modelDiagonal = std::max(moving.bounds.diagonal(), target.bounds.diagonal());
  if (!std::isfinite(modelDiagonal))
    return invalid("surface coordinates are too large to process safely");
  const double tolerance = request.tolerance > 0.0
    ? request.tolerance : std::max(1e-12, modelDiagonal * 1e-9);
  TriangleBvh targetBvh(target.triangles);
  const bool movingClosed = vtkSelectEnclosedPoints::IsSurfaceClosed(moving.data) != 0;
  const bool targetClosed = vtkSelectEnclosedPoints::IsSurfaceClosed(target.data) != 0;
  if ((movingClosed || targetClosed) &&
      hasInteriorOverlap(moving, target, tolerance, targetBvh, movingClosed, targetClosed)) {
    result.status = ApproachStatus::AlreadyPenetrating;
    result.diagnostic = "moving and target surfaces already overlap volumetrically";
    return result;
  }

  if (!movingClosed || !targetClosed)
    appendDiagnostic(result.diagnostic,
      "one or both surfaces are open; volumetric initial penetration cannot be fully classified");
  if (!moving.diagnostic.empty()) appendDiagnostic(result.diagnostic, moving.diagnostic);
  if (!target.diagnostic.empty()) appendDiagnostic(result.diagnostic, target.diagnostic);

  std::vector<std::size_t> candidates;
  double firstContact = std::numeric_limits<double>::infinity();
  for (const Triangle& movingTri : moving.triangles) {
    candidates.clear();
    targetBvh.query(sweptBounds(movingTri, direction, request.maximumMovement, tolerance), candidates);
    result.broadPhaseCandidatePairCount += candidates.size();
    for (std::size_t targetIndex : candidates) {
      ++result.narrowPhasePairCount;
      double enter = 0.0, exit = 0.0;
      if (!sweepTriangles(movingTri, target.triangles[targetIndex], direction, tolerance,
                          request.maximumMovement, enter, exit)) continue;
      if (enter > request.maximumMovement + tolerance) continue;
      if (enter < firstContact - tolerance) {
        firstContact = enter;
        result.movingContactFaces.clear();
        result.targetContactFaces.clear();
      } else if (enter > firstContact + tolerance) {
        continue;
      } else {
        firstContact = std::min(firstContact, enter);
      }
      addUnique(result.movingContactFaces, movingTri.originalCell);
      addUnique(result.targetContactFaces, target.triangles[targetIndex].originalCell);
    }
  }

  std::sort(result.movingContactFaces.begin(), result.movingContactFaces.end());
  std::sort(result.targetContactFaces.begin(), result.targetContactFaces.end());

  if (!std::isfinite(firstContact)) {
    result.status = ApproachStatus::NoContactWithinLimit;
    appendDiagnostic(result.diagnostic, "no contact within maximum movement");
    return result;
  }
  result.contactDistance = firstContact;
  const double rawTranslationDistance = firstContact - request.targetGap;
  if (rawTranslationDistance < -tolerance) {
    result.status = ApproachStatus::ClearanceExceedsGap;
    appendDiagnostic(result.diagnostic, "requested target gap exceeds the current directional gap");
    return result;
  }
  result.translationDistance = std::max(0.0, rawTranslationDistance);
  result.translation = {{direction.x * result.translationDistance,
                         direction.y * result.translationDistance,
                         direction.z * result.translationDistance}};
  result.status = ApproachStatus::Success;
  return result;
}
