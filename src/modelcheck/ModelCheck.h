#ifndef WFGUI_MODELCHECK_MODELCHECK_H
#define WFGUI_MODELCHECK_MODELCHECK_H

#include <string>
#include <vector>

class Model;

namespace wfgui {
namespace modelcheck {

enum class CheckSeverity {
  Info,
  Warning,
  Error
};

enum class CheckCategory {
  Geometry,
  Mesh,
  Materials,
  Sections,
  Sets,
  BoundaryConditions,
  Steps,
  Contacts,
  ExportCompatibility
};

enum class CheckProfile {
  General,
  WeldForm,
  Abaqus,
  OpenRadioss
};

struct CheckIssue {
  CheckSeverity severity = CheckSeverity::Info;
  CheckCategory category = CheckCategory::Geometry;
  std::string code;
  std::string message;
  int entityId = -1;
};

struct CheckContext {
  Model* model = nullptr;
  CheckProfile profile = CheckProfile::General;
};

struct CheckReport {
  CheckProfile profile = CheckProfile::General;
  std::vector<CheckIssue> issues;
  int infoCount = 0;
  int warningCount = 0;
  int errorCount = 0;
};

class ModelChecker {
public:
  static CheckReport run(const CheckContext& context);
};

bool hasErrors(const CheckReport& report);
const char* toString(CheckSeverity severity);
const char* toString(CheckCategory category);
const char* toString(CheckProfile profile);
std::string formatIssue(const CheckIssue& issue);
std::string formatSummary(const CheckReport& report);

}  // namespace modelcheck
}  // namespace wfgui

#endif
