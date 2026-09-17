#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <vector>

#include <vtkType.h>
class vtkPolyData;

struct ApproachToContactRequest {
  vtkPolyData* movingSurface = nullptr;
  vtkPolyData* targetSurface = nullptr;
  std::array<double, 3> direction{{0.0, 0.0, 0.0}};
  double targetGap = 0.0;
  double maximumMovement = 0.0;
  double tolerance = 0.0;
};

enum class ApproachStatus {
  Success,
  InvalidInput,
  AlreadyPenetrating,
  NoContactWithinLimit,
  ClearanceExceedsGap
};

struct ApproachToContactResult {
  ApproachStatus status = ApproachStatus::InvalidInput;
  double contactDistance = 0.0;
  double translationDistance = 0.0;
  std::array<double, 3> translation{{0.0, 0.0, 0.0}};
  std::vector<vtkIdType> movingContactFaces;
  std::vector<vtkIdType> targetContactFaces;
  std::size_t broadPhaseCandidatePairCount = 0;
  std::size_t narrowPhasePairCount = 0;
  std::string diagnostic;
};

// Computes the first contact while translating the moving surface along the
// normalized request direction. Inputs are read-only; all triangulation and
// acceleration structures are built from private copies.
ApproachToContactResult computeApproachToContact(
    const ApproachToContactRequest& request);
