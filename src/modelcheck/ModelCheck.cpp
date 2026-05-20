#include "modelcheck/ModelCheck.h"

#include "Model.h"
#include "Part.h"
#include "Mesh.h"
#include "Node.h"
#include "Element.h"
#include "ElementType.h"
#include "Set.h"
#include "BoundaryCondition.h"
#include "InitialCondition.h"
#include "Step.h"

#include <map>
#include <set>
#include <sstream>
#include <vector>

namespace wfgui {
namespace modelcheck {
namespace {

Step* activeStep(Model* model)
{
  if (model == nullptr || model->getStepCount() == 0) {
    return nullptr;
  }
  return model->getStep(0);
}

Part* findPartById(Model* model, int partId)
{
  if (model == nullptr) {
    return nullptr;
  }

  for (int i = 0; i < model->getPartCount(); ++i) {
    Part* part = model->getPart(i);
    if (part != nullptr && part->getId() == partId) {
      return part;
    }
  }

  return nullptr;
}

NodeSet* findNodeSetById(Model* model, int setId)
{
  if (model == nullptr) {
    return nullptr;
  }

  for (int i = 0; i < model->getPartCount(); ++i) {
    Part* part = model->getPart(i);
    if (part == nullptr || part->getMesh() == nullptr) {
      continue;
    }

    NodeSet* set = part->getMesh()->findNodeSetById(setId);
    if (set != nullptr) {
      return set;
    }
  }

  return nullptr;
}

void addIssue(CheckReport& report,
              CheckSeverity severity,
              CheckCategory category,
              const std::string& code,
              const std::string& message,
              int entityId = -1)
{
  CheckIssue issue;
  issue.severity = severity;
  issue.category = category;
  issue.code = code;
  issue.message = message;
  issue.entityId = entityId;
  report.issues.push_back(issue);

  if (severity == CheckSeverity::Error) {
    ++report.errorCount;
  } else if (severity == CheckSeverity::Warning) {
    ++report.warningCount;
  } else {
    ++report.infoCount;
  }
}

void runGeneralChecks(const CheckContext& context, CheckReport& report)
{
  Model* model = context.model;
  if (model == nullptr) {
    addIssue(report,
             CheckSeverity::Error,
             CheckCategory::Geometry,
             "MODEL_NULL",
             "Active model pointer is null.");
    return;
  }

  if (model->getPartCount() == 0) {
    addIssue(report,
             CheckSeverity::Error,
             CheckCategory::Geometry,
             "MODEL_NO_PARTS",
             "Model has no parts.");
  }

  Step* step = activeStep(model);
  if (step == nullptr) {
    addIssue(report,
             CheckSeverity::Error,
             CheckCategory::Steps,
             "STEP_MISSING",
             "Model has no analysis step.");
  }

  std::map<int, int> partIdCounts;
  std::map<int, int> nodeSetIdCounts;
  std::map<int, int> elementSetIdCounts;
  std::map<int, int> faceSetIdCounts;
  int meshedDeformablePartCount = 0;

  for (int partIndex = 0; partIndex < model->getPartCount(); ++partIndex) {
    Part* part = model->getPart(partIndex);
    if (part == nullptr) {
      addIssue(report,
               CheckSeverity::Warning,
               CheckCategory::Geometry,
               "PART_NULL",
               "Model contains a null part entry.");
      continue;
    }

    ++partIdCounts[part->getId()];

    if (!part->isMeshed() || part->getMesh() == nullptr) {
      addIssue(report,
               CheckSeverity::Error,
               CheckCategory::Mesh,
               "PART_WITHOUT_MESH",
               "Part " + std::to_string(part->getId()) + " has no mesh.",
               part->getId());
      continue;
    }

    Mesh* mesh = part->getMesh();
    if (mesh->getNodeCount() == 0) {
      addIssue(report,
               CheckSeverity::Error,
               CheckCategory::Mesh,
               "MESH_NO_NODES",
               "Part " + std::to_string(part->getId()) + " mesh has no nodes.",
               part->getId());
    }
    if (mesh->getElemCount() == 0) {
      addIssue(report,
               CheckSeverity::Error,
               CheckCategory::Mesh,
               "MESH_NO_ELEMENTS",
               "Part " + std::to_string(part->getId()) + " mesh has no elements.",
               part->getId());
    }

    if (part->getType() == Elastic) {
      ++meshedDeformablePartCount;
    }

    int bulkElementCount = 0;
    int boundaryElementCount = 0;
    int unknownElementCount = 0;

    std::set<int> referencedNodeIds;
    for (int elemIndex = 0; elemIndex < mesh->getElemCount(); ++elemIndex) {
      Element* element = mesh->getElem(elemIndex);
      if (element == nullptr) {
        addIssue(report,
                 CheckSeverity::Warning,
                 CheckCategory::Mesh,
                 "ELEMENT_NULL",
                 "Part " + std::to_string(part->getId()) + " contains a null element entry.",
                 part->getId());
        continue;
      }

      const ElementType elementType = inferElementType(mesh, element);
      const ElementUsage elementUsage = inferElementUsage(mesh, element);
      if (elementUsage == ElementUsage::Bulk) {
        ++bulkElementCount;
      } else if (elementUsage == ElementUsage::Boundary) {
        ++boundaryElementCount;
      } else {
        ++unknownElementCount;
        addIssue(report,
                 CheckSeverity::Warning,
                 CheckCategory::Mesh,
                 "ELEMENT_TYPE_UNKNOWN",
                 "Part " + std::to_string(part->getId()) + " contains unsupported or ambiguous element topology " +
                   std::string(toString(elementType)) + ".",
                 element->getId());
      }

      for (int nodeIndex = 0; nodeIndex < element->getNodeCount(); ++nodeIndex) {
        referencedNodeIds.insert(element->getNodeId(nodeIndex));
      }
    }

    if (part->getType() == Elastic) {
      if (bulkElementCount == 0 && mesh->getElemCount() > 0) {
        addIssue(report,
                 CheckSeverity::Error,
                 CheckCategory::Mesh,
                 "DEFORMABLE_NO_BULK_ELEMENTS",
                 "Part " + std::to_string(part->getId()) + " is deformable but has no bulk elements.",
                 part->getId());
      }
      if (boundaryElementCount > 0 && bulkElementCount > 0) {
        addIssue(report,
                 CheckSeverity::Warning,
                 CheckCategory::Mesh,
                 "DEFORMABLE_MIXED_BULK_BOUNDARY",
                 "Part " + std::to_string(part->getId()) + " mixes bulk and boundary elements in the same mesh.",
                 part->getId());
      }
    } else {
      if (bulkElementCount > 0) {
        addIssue(report,
                 CheckSeverity::Error,
                 CheckCategory::Mesh,
                 "RIGID_CONTAINS_BULK_ELEMENTS",
                 "Rigid part " + std::to_string(part->getId()) + " contains bulk elements.",
                 part->getId());
      }
    }

    for (int nodeIndex = 0; nodeIndex < mesh->getNodeCount(); ++nodeIndex) {
      Node* node = mesh->getNode(nodeIndex);
      if (node == nullptr) {
        addIssue(report,
                 CheckSeverity::Warning,
                 CheckCategory::Mesh,
                 "NODE_NULL",
                 "Part " + std::to_string(part->getId()) + " contains a null node entry.",
                 part->getId());
        continue;
      }

      if (referencedNodeIds.find(node->getId()) == referencedNodeIds.end()) {
        addIssue(report,
                 CheckSeverity::Warning,
                 CheckCategory::Mesh,
                 "ORPHAN_NODE",
                 "Node " + std::to_string(node->getId()) + " in part " +
                   std::to_string(part->getId()) + " is not referenced by any element.",
                 node->getId());
      }
    }

    for (int setIndex = 0; setIndex < mesh->getNodeSetCount(); ++setIndex) {
      const NodeSet& set = mesh->getNodeSet(setIndex);
      ++nodeSetIdCounts[set.getId()];
      if (set.getItemCount() == 0) {
        addIssue(report,
                 CheckSeverity::Warning,
                 CheckCategory::Sets,
                 "NODE_SET_EMPTY",
                 "Node set " + std::to_string(set.getId()) + " is empty.",
                 set.getId());
      }
    }

    for (int setIndex = 0; setIndex < mesh->getElementSetCount(); ++setIndex) {
      const ElementSet& set = mesh->getElementSet(setIndex);
      ++elementSetIdCounts[set.getId()];
      if (set.getItemCount() == 0) {
        addIssue(report,
                 CheckSeverity::Warning,
                 CheckCategory::Sets,
                 "ELEMENT_SET_EMPTY",
                 "Element set " + std::to_string(set.getId()) + " is empty.",
                 set.getId());
      }
    }

    for (int setIndex = 0; setIndex < mesh->getFaceSetCount(); ++setIndex) {
      const FaceSet& set = mesh->getFaceSet(setIndex);
      ++faceSetIdCounts[set.getId()];
      if (set.getItemCount() == 0) {
        addIssue(report,
                 CheckSeverity::Warning,
                 CheckCategory::Sets,
                 "FACE_SET_EMPTY",
                 "Face set " + std::to_string(set.getId()) + " is empty.",
                 set.getId());
      }
    }
  }

  for (std::map<int, int>::const_iterator it = partIdCounts.begin(); it != partIdCounts.end(); ++it) {
    if (it->second > 1) {
      addIssue(report,
               CheckSeverity::Error,
               CheckCategory::Geometry,
               "PART_ID_DUPLICATE",
               "Duplicate part id " + std::to_string(it->first) + ".",
               it->first);
    }
  }

  for (std::map<int, int>::const_iterator it = nodeSetIdCounts.begin(); it != nodeSetIdCounts.end(); ++it) {
    if (it->second > 1) {
      addIssue(report,
               CheckSeverity::Error,
               CheckCategory::Sets,
               "NODE_SET_ID_DUPLICATE",
               "Duplicate node set id " + std::to_string(it->first) + ".",
               it->first);
    }
  }

  for (std::map<int, int>::const_iterator it = elementSetIdCounts.begin(); it != elementSetIdCounts.end(); ++it) {
    if (it->second > 1) {
      addIssue(report,
               CheckSeverity::Error,
               CheckCategory::Sets,
               "ELEMENT_SET_ID_DUPLICATE",
               "Duplicate element set id " + std::to_string(it->first) + ".",
               it->first);
    }
  }

  for (std::map<int, int>::const_iterator it = faceSetIdCounts.begin(); it != faceSetIdCounts.end(); ++it) {
    if (it->second > 1) {
      addIssue(report,
               CheckSeverity::Error,
               CheckCategory::Sets,
               "FACE_SET_ID_DUPLICATE",
               "Duplicate face set id " + std::to_string(it->first) + ".",
               it->first);
    }
  }

  if (meshedDeformablePartCount > 0 && model->getMaterialCount() == 0) {
    addIssue(report,
             CheckSeverity::Error,
             CheckCategory::Materials,
             "MATERIAL_MISSING",
             "Model has deformable meshed parts but no material defined.");
  }

  for (int bcIndex = 0; bcIndex < model->getBCCount(); ++bcIndex) {
    BoundaryCondition* bc = model->getBC(bcIndex);
    if (bc == nullptr) {
      addIssue(report,
               CheckSeverity::Warning,
               CheckCategory::BoundaryConditions,
               "BC_NULL",
               "Model contains a null boundary condition entry.");
      continue;
    }

    if (bc->getApplyTo() == ApplyToPart) {
      Part* part = findPartById(model, bc->getTargetId());
      if (part == nullptr) {
        addIssue(report,
                 CheckSeverity::Error,
                 CheckCategory::BoundaryConditions,
                 "BC_PART_MISSING",
                 "Boundary condition references missing part id " +
                   std::to_string(bc->getTargetId()) + ".",
                 bc->getTargetId());
      }
    } else {
      NodeSet* set = findNodeSetById(model, bc->getTargetId());
      if (set == nullptr) {
        addIssue(report,
                 CheckSeverity::Error,
                 CheckCategory::BoundaryConditions,
                 "BC_SET_MISSING",
                 "Boundary condition references missing node set id " +
                   std::to_string(bc->getTargetId()) + ".",
                 bc->getTargetId());
      } else if (set->getItemCount() == 0) {
        addIssue(report,
                 CheckSeverity::Error,
                 CheckCategory::BoundaryConditions,
                 "BC_SET_EMPTY",
                 "Boundary condition references empty node set id " +
                   std::to_string(bc->getTargetId()) + ".",
                 bc->getTargetId());
      }
    }
  }

  for (int icIndex = 0; icIndex < model->getICCount(); ++icIndex) {
    InitialCondition* ic = model->getIC(icIndex);
    if (ic == nullptr) {
      addIssue(report,
               CheckSeverity::Warning,
               CheckCategory::BoundaryConditions,
               "IC_NULL",
               "Model contains a null initial condition entry.");
      continue;
    }

    if (ic->getApplyTo() == ApplyToPart) {
      Part* part = findPartById(model, ic->getTargetId());
      if (part == nullptr) {
        addIssue(report,
                 CheckSeverity::Error,
                 CheckCategory::BoundaryConditions,
                 "IC_PART_MISSING",
                 "Initial condition references missing part id " +
                   std::to_string(ic->getTargetId()) + ".",
                 ic->getTargetId());
      }
    } else {
      NodeSet* set = findNodeSetById(model, ic->getTargetId());
      if (set == nullptr) {
        addIssue(report,
                 CheckSeverity::Error,
                 CheckCategory::BoundaryConditions,
                 "IC_SET_MISSING",
                 "Initial condition references missing node set id " +
                   std::to_string(ic->getTargetId()) + ".",
                 ic->getTargetId());
      } else if (set->getItemCount() == 0) {
        addIssue(report,
                 CheckSeverity::Error,
                 CheckCategory::BoundaryConditions,
                 "IC_SET_EMPTY",
                 "Initial condition references empty node set id " +
                   std::to_string(ic->getTargetId()) + ".",
                 ic->getTargetId());
      }
    }
  }
}

void runWeldFormChecks(const CheckContext& context, CheckReport& report)
{
  Model* model = context.model;
  if (model == nullptr) {
    return;
  }

  Step* step = activeStep(model);
  const bool implicit = (step != nullptr && step->isImplicit());
  if (implicit) {
    return;
  }

  int elasticPartCount = 0;
  for (int partIndex = 0; partIndex < model->getPartCount(); ++partIndex) {
    Part* part = model->getPart(partIndex);
    if (part == nullptr || !part->isMeshed() || part->getMesh() == nullptr) {
      continue;
    }

    Mesh* mesh = part->getMesh();
    if (part->getType() == Elastic) {
      ++elasticPartCount;

      for (int elemIndex = 0; elemIndex < mesh->getElemCount(); ++elemIndex) {
        Element* element = mesh->getElem(elemIndex);
        if (element == nullptr) {
          continue;
        }

        const ElementType elementType = inferElementType(mesh, element);
        if (implicit && elementType != ElementType::Quad4) {
          addIssue(report,
                   CheckSeverity::Error,
                   CheckCategory::ExportCompatibility,
                   "WELDFORM_IMPLICIT_EXPECTS_QUAD4",
                   "Implicit WeldForm currently expects deformable Quad4 elements; part " +
                     std::to_string(part->getId()) + " contains " + toString(elementType) + ".",
                   part->getId());
          break;
        }
      }
    } else {
      for (int elemIndex = 0; elemIndex < mesh->getElemCount(); ++elemIndex) {
        Element* element = mesh->getElem(elemIndex);
        if (element == nullptr) {
          continue;
        }

        const ElementType elementType = inferElementType(mesh, element);
        if (mesh->getDim() == 2 && elementType != ElementType::Line2) {
          addIssue(report,
                   CheckSeverity::Error,
                   CheckCategory::ExportCompatibility,
                   "WELDFORM_RIGID_2D_EXPECTS_LINE2",
                   "Rigid 2D WeldForm contact currently expects Line2 elements; part " +
                     std::to_string(part->getId()) + " contains " + toString(elementType) + ".",
                   part->getId());
          break;
        }
        if (mesh->getDim() == 3 && elementType != ElementType::Tria3) {
          addIssue(report,
                   CheckSeverity::Error,
                   CheckCategory::ExportCompatibility,
                   "WELDFORM_RIGID_3D_EXPECTS_TRIA3",
                   "Rigid 3D WeldForm contact currently expects Tria3 elements; part " +
                     std::to_string(part->getId()) + " contains " + toString(elementType) + ".",
                   part->getId());
          break;
        }
      }
    }
  }

  if (elasticPartCount > 1) {
    addIssue(report,
             CheckSeverity::Error,
             CheckCategory::ExportCompatibility,
             "WELDFORM_EXPLICIT_MULTI_ELASTIC",
             "Current explicit WeldForm export supports only one elastic meshed body.");
  }
}

}  // namespace

CheckReport ModelChecker::run(const CheckContext& context)
{
  CheckReport report;
  report.profile = context.profile;

  runGeneralChecks(context, report);

  if (context.profile == CheckProfile::WeldForm) {
    runWeldFormChecks(context, report);
  }

  return report;
}

bool hasErrors(const CheckReport& report)
{
  return report.errorCount > 0;
}

const char* toString(CheckSeverity severity)
{
  switch (severity) {
    case CheckSeverity::Info:
      return "Info";
    case CheckSeverity::Warning:
      return "Warning";
    case CheckSeverity::Error:
      return "Error";
  }
  return "Info";
}

const char* toString(CheckCategory category)
{
  switch (category) {
    case CheckCategory::Geometry:
      return "Geometry";
    case CheckCategory::Mesh:
      return "Mesh";
    case CheckCategory::Materials:
      return "Materials";
    case CheckCategory::Sections:
      return "Sections";
    case CheckCategory::Sets:
      return "Sets";
    case CheckCategory::BoundaryConditions:
      return "Boundary Conditions";
    case CheckCategory::Steps:
      return "Steps";
    case CheckCategory::Contacts:
      return "Contacts";
    case CheckCategory::ExportCompatibility:
      return "Export Compatibility";
  }
  return "Geometry";
}

const char* toString(CheckProfile profile)
{
  switch (profile) {
    case CheckProfile::General:
      return "General";
    case CheckProfile::WeldForm:
      return "WeldForm";
  }
  return "General";
}

std::string formatIssue(const CheckIssue& issue)
{
  std::ostringstream oss;
  oss << "[" << toString(issue.severity) << "] "
      << toString(issue.category) << " / " << issue.code << ": "
      << issue.message;
  if (issue.entityId >= 0) {
    oss << " (id=" << issue.entityId << ")";
  }
  return oss.str();
}

std::string formatSummary(const CheckReport& report)
{
  std::ostringstream oss;
  oss << "Model Check [" << toString(report.profile) << "]: "
      << report.errorCount << " error(s), "
      << report.warningCount << " warning(s), "
      << report.infoCount << " info message(s).";
  return oss.str();
}

}  // namespace modelcheck
}  // namespace wfgui
