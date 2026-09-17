#include "ApproachToContact.h"

#include <vtkCellArray.h>
#include <vtkCubeSource.h>
#include <vtkNew.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {
using Point = std::array<double, 3>;
using Tri = std::array<Point, 3>;

int failures = 0;

void check(bool condition, const std::string& message) {
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
  }
}

bool near(double actual, double expected, double tolerance = 1e-7) {
  return std::abs(actual - expected) <= tolerance;
}

vtkSmartPointer<vtkPolyData> surface(const std::vector<Tri>& triangles) {
  vtkNew<vtkPoints> points;
  points->SetDataTypeToDouble();
  vtkNew<vtkCellArray> polys;
  for (const Tri& triangle : triangles) {
    vtkIdType ids[3];
    for (int i = 0; i < 3; ++i)
      ids[i] = points->InsertNextPoint(triangle[i].data());
    polys->InsertNextCell(3, ids);
  }
  vtkSmartPointer<vtkPolyData> result = vtkSmartPointer<vtkPolyData>::New();
  result->SetPoints(points);
  result->SetPolys(polys);
  return result;
}

Tri triangle(double z, double x = 0.0, double size = 1.0) {
  return {{{x, 0.0, z}, {x + size, 0.0, z}, {x, size, z}}};
}

ApproachToContactResult run(vtkPolyData* moving, vtkPolyData* target,
                            std::array<double, 3> direction = {{0.0, 0.0, -1.0}},
                            double gap = 0.0, double maxMove = 10.0) {
  ApproachToContactRequest request;
  request.movingSurface = moving;
  request.targetSurface = target;
  request.direction = direction;
  request.targetGap = gap;
  request.maximumMovement = maxMove;
  return computeApproachToContact(request);
}

vtkSmartPointer<vtkPolyData> cube(double centerZ) {
  vtkNew<vtkCubeSource> source;
  source->SetCenter(0.0, 0.0, centerZ);
  source->SetXLength(2.0);
  source->SetYLength(2.0);
  source->SetZLength(2.0);
  source->Update();
  vtkSmartPointer<vtkPolyData> result = vtkSmartPointer<vtkPolyData>::New();
  result->DeepCopy(source->GetOutput());
  return result;
}

void testParallelPlanesAndDirectionNormalization() {
  auto moving = surface({triangle(2.0)});
  auto target = surface({triangle(0.0)});
  auto unit = run(moving, target);
  auto scaled = run(moving, target, {{0.0, 0.0, -20.0}});
  check(unit.status == ApproachStatus::Success, "parallel triangles produce a contact");
  check(near(unit.contactDistance, 2.0), "parallel triangle contact distance is 2");
  check(near(unit.translation[2], -2.0), "parallel triangle translation is correct");
  check(scaled.status == ApproachStatus::Success && near(scaled.contactDistance, unit.contactDistance),
        "direction magnitude does not affect the result");
}

void testInclinedDirectionalContactAndTargetGap() {
  auto moving = surface({triangle(2.0, 0.0, 0.1)});
  auto target = surface({Tri{{{-10.0, -10.0, 0.0}, {10.0, -10.0, 0.0}, {0.0, 10.0, 0.0}}}});
  const std::array<double, 3> direction{{1.0, 0.0, -1.0}};
  auto result = run(moving, target, direction, 0.25);
  check(result.status == ApproachStatus::Success, "inclined translation finds contact");
  check(near(result.contactDistance, std::sqrt(8.0)), "inclined contact uses directional distance");
  check(near(result.translationDistance, std::sqrt(8.0) - 0.25), "target gap is left before contact");
  check(near(result.translation[0], (std::sqrt(8.0) - 0.25) / std::sqrt(2.0)),
        "inclined translation vector is normalized");
}

void testSimultaneousAndEarlierRegions() {
  auto moving = surface({triangle(2.0, 0.0), triangle(2.0, 3.0)});
  auto target = surface({triangle(0.0, 0.0), triangle(0.0, 3.0)});
  auto simultaneous = run(moving, target);
  check(simultaneous.status == ApproachStatus::Success, "two simultaneous contact regions succeed");
  check(simultaneous.movingContactFaces.size() == 2 && simultaneous.targetContactFaces.size() == 2,
        "simultaneous contact reports both original face IDs");

  auto staggeredMoving = surface({triangle(2.0), triangle(3.0)});
  auto oneTarget = surface({triangle(0.0)});
  auto earliest = run(staggeredMoving, oneTarget);
  check(earliest.status == ApproachStatus::Success && near(earliest.contactDistance, 2.0),
        "earliest region wins over a later region");
  check(earliest.movingContactFaces.size() == 1 && earliest.movingContactFaces[0] == 0,
        "later region is excluded from first contact candidates");
}

void testEdgeToEdgeContact() {
  const Tri movingTriangle{{
    {{-0.38338681484136194, 0.7268590379585531, 0.8435396488725246}},
    {{ 0.02798966515491852,-0.7129011536205871,-0.7918246545075276}},
    {{ 0.6358578317597556, 0.7759216276384286, 0.30774470605796833}}
  }};
  const Tri targetTriangle{{
    {{ 0.5477278515411343, 0.5873172321535649,-0.5257405606401735}},
    {{-0.5735385038763219, 0.7352347876034269, 0.6784943003455226}},
    {{-0.12767075932844962,-0.45952229283906054,-0.8425047649784514}}
  }};
  auto moving = surface({movingTriangle});
  auto target = surface({targetTriangle});
  auto result = run(moving, target, {{-0.1858595619284148, 0.795405678084546, -0.5768760963245415}});
  check(result.status == ApproachStatus::Success, "edge-to-edge sweep contact succeeds");
  check(near(result.contactDistance, 0.24634078097322099, 2e-7),
        "edge-to-edge SAT axes determine the first collision");
}

void testTouchingPenetrationAndClearance() {
  auto target = cube(0.0);
  auto touching = cube(2.0);
  auto tangent = run(touching, target, {{0.0, 0.0, 1.0}});
  check(tangent.status == ApproachStatus::Success && near(tangent.contactDistance, 0.0),
        "initial tangency is contact at zero distance, even moving away");

  auto overlapping = cube(0.5);
  auto penetration = run(overlapping, target);
  check(penetration.status == ApproachStatus::AlreadyPenetrating,
        "closed overlapping solids are rejected as already penetrating");
  auto openToolInside = surface({triangle(0.0, -0.25, 0.5)});
  check(run(openToolInside, target).status == ApproachStatus::AlreadyPenetrating,
        "an open rigid surface inside a closed target is rejected as penetrating");

  vtkNew<vtkCubeSource> cornerOverlapSource;
  cornerOverlapSource->SetCenter(1.5, 1.5, 0.0);
  cornerOverlapSource->SetXLength(2.0);
  cornerOverlapSource->SetYLength(2.0);
  cornerOverlapSource->SetZLength(2.0);
  cornerOverlapSource->Update();
  vtkSmartPointer<vtkPolyData> cornerOverlap = vtkSmartPointer<vtkPolyData>::New();
  cornerOverlap->DeepCopy(cornerOverlapSource->GetOutput());
  check(run(cornerOverlap, target).status == ApproachStatus::AlreadyPenetrating,
        "crossing surfaces with no interior vertices are still classified as penetration");

  auto planesMoving = surface({triangle(1.0)});
  auto planeTarget = surface({triangle(0.0)});
  auto excessiveGap = run(planesMoving, planeTarget, {{0.0, 0.0, -1.0}}, 1.2);
  check(excessiveGap.status == ApproachStatus::ClearanceExceedsGap,
        "clearance larger than current gap is not applied backwards");
}

void testMovementLimitAndInvalidInputs() {
  auto moving = surface({triangle(2.0)});
  auto target = surface({triangle(0.0)});
  auto limited = run(moving, target, {{0.0, 0.0, -1.0}}, 0.0, 1.0);
  check(limited.status == ApproachStatus::NoContactWithinLimit,
        "contact beyond maximum movement is reported");

  auto zeroDirection = run(moving, target, {{0.0, 0.0, 0.0}});
  check(zeroDirection.status == ApproachStatus::InvalidInput,
        "zero direction is rejected");
  auto negativeGap = run(moving, target, {{0.0, 0.0, -1.0}}, -0.1);
  check(negativeGap.status == ApproachStatus::InvalidInput, "negative target gap is rejected");
  auto degenerate = surface({Tri{{{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {2.0, 0.0, 0.0}}}});
  auto noTriangles = run(degenerate, target);
  check(noTriangles.status == ApproachStatus::InvalidInput,
        "all-degenerate input surface is rejected");
  auto partlyDegenerate = surface({
    Tri{{{0.0, 0.0, 1.0}, {1.0, 0.0, 1.0}, {2.0, 0.0, 1.0}}},
    triangle(2.0)
  });
  auto validRemainder = run(partlyDegenerate, target);
  check(validRemainder.status == ApproachStatus::Success &&
        validRemainder.movingContactFaces.size() == 1 &&
        validRemainder.movingContactFaces[0] == 1,
        "degenerate polygons are skipped while original IDs remain stable");
  check(validRemainder.diagnostic.find("degenerate") != std::string::npos,
        "skipped degenerate input is reported in diagnostics");
  auto empty = surface({});
  check(run(empty, target).status == ApproachStatus::InvalidInput, "empty surface is rejected");
}

void testSpatialBroadPhaseScales() {
  std::vector<Tri> movingTriangles;
  std::vector<Tri> targetTriangles;
  constexpr int count = 400;
  for (int i = 0; i < count; ++i) {
    movingTriangles.push_back(triangle(1.0, i * 3.0, 0.5));
    targetTriangles.push_back(triangle(0.0, i * 3.0, 0.5));
  }
  auto moving = surface(movingTriangles);
  auto target = surface(targetTriangles);
  auto result = run(moving, target, {{0.0, 0.0, -1.0}}, 0.0, 2.0);
  check(result.status == ApproachStatus::Success, "large synthetic surfaces find contacts");
  check(result.narrowPhasePairCount < static_cast<std::size_t>(count * count / 20),
        "BVH avoids near-quadratic narrow-phase pair testing");
  check(result.broadPhaseCandidatePairCount < static_cast<std::size_t>(count * count / 20),
        "swept AABB candidate generation scales below the all-pairs count");
}

} // namespace

int main() {
  testParallelPlanesAndDirectionNormalization();
  testInclinedDirectionalContactAndTargetGap();
  testSimultaneousAndEarlierRegions();
  testEdgeToEdgeContact();
  testTouchingPenetrationAndClearance();
  testMovementLimitAndInvalidInputs();
  testSpatialBroadPhaseScales();
  if (failures != 0) {
    std::cerr << failures << " test(s) failed\n";
    return EXIT_FAILURE;
  }
  std::cout << "All approach-to-contact geometry tests passed\n";
  return EXIT_SUCCESS;
}
