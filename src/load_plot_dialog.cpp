#include "load_plot_dialog.h"

#include "imgui.h"
#include "implot.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>

namespace {

std::string Trim(const std::string& value) {
  const std::string whitespace = " \t\r\n";
  const std::size_t begin = value.find_first_not_of(whitespace);
  if (begin == std::string::npos) {
    return "";
  }
  const std::size_t end = value.find_last_not_of(whitespace);
  return value.substr(begin, end - begin + 1);
}

std::vector<std::string> SplitCsvLine(const std::string& line) {
  std::vector<std::string> tokens;
  std::stringstream stream(line);
  std::string token;
  while (std::getline(stream, token, ',')) {
    tokens.push_back(Trim(token));
  }
  if (!line.empty() && line.back() == ',') {
    tokens.emplace_back();
  }
  return tokens;
}

bool TryParseDouble(const std::string& token, double& value) {
  const std::string trimmed = Trim(token);
  if (trimmed.empty()) {
    return false;
  }

  char* end_ptr = nullptr;
  errno = 0;
  const double parsed = std::strtod(trimmed.c_str(), &end_ptr);
  if (end_ptr == trimmed.c_str() || *end_ptr != '\0') {
    return false;
  }

  // Accept subnormal/underflowed values such as 2.4e-311, but still reject
  // true overflow/non-finite results.
  if (errno == ERANGE && !std::isfinite(parsed)) {
    return false;
  }

  value = parsed;
  return true;
}

bool IsLoadSeriesColumn(const std::string& token) {
  const std::string trimmed = Trim(token);
  if (trimmed.size() < 2) {
    return false;
  }

  const char first = static_cast<char>(std::tolower(static_cast<unsigned char>(trimmed[0])));
  if (first != 'f') {
    return false;
  }

  std::size_t i = 1;
  while (i < trimmed.size() &&
         std::isdigit(static_cast<unsigned char>(trimmed[i]))) {
    ++i;
  }

  if (i == 1) {
    return false;
  }

  if (i == trimmed.size()) {
    return true;
  }

  if (trimmed[i] != '_') {
    return false;
  }

  ++i;
  if (i >= trimmed.size()) {
    return false;
  }

  for (; i < trimmed.size(); ++i) {
    if (!std::isdigit(static_cast<unsigned char>(trimmed[i]))) {
      const char c = static_cast<char>(std::tolower(static_cast<unsigned char>(trimmed[i])));
      if (c != '_' && (c < 'a' || c > 'z')) {
        return false;
      }
    }
  }
  return true;
}

bool IsSetSeriesColumn(const std::string& token) {
  const std::string trimmed = Trim(token);
  if (trimmed.size() < 5) {
    return false;
  }

  std::string lower = trimmed;
  std::transform(lower.begin(), lower.end(), lower.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  if (lower.rfind("set", 0) != 0) {
    return false;
  }

  std::size_t i = 3;
  while (i < lower.size() && std::isdigit(static_cast<unsigned char>(lower[i]))) {
    ++i;
  }
  if (i == 3 || i >= lower.size() || lower[i] != '_') {
    return false;
  }
  ++i;
  if (i >= lower.size()) {
    return false;
  }

  for (; i < lower.size(); ++i) {
    const char c = lower[i];
    if (c != '_' && (c < 'a' || c > 'z')) {
      return false;
    }
  }
  return true;
}

bool IsSupportedLoadSeriesColumn(const std::string& token) {
  return IsLoadSeriesColumn(token) || IsSetSeriesColumn(token);
}

struct LoadedCsvData {
  std::string x_label = "Time";
  std::vector<std::string> series_names;
  std::vector<double> time_values;
  std::vector<std::vector<double>> series_values;
};

bool LoadPlotCsvFile(const std::string& csv_path,
                     LoadedCsvData& data,
                     std::string& error_message) {
  std::ifstream file(csv_path);
  if (!file.is_open()) {
    error_message = "Could not open CSV file: " + csv_path;
    return false;
  }

  std::string line;
  bool header_processed = false;
  int line_number = 0;
  std::vector<std::size_t> selected_series_columns;

  while (std::getline(file, line)) {
    ++line_number;
    const std::string trimmed_line = Trim(line);
    if (trimmed_line.empty()) {
      continue;
    }

    std::vector<std::string> tokens = SplitCsvLine(trimmed_line);
    if (tokens.size() < 2) {
      continue;
    }

    std::vector<double> numeric_values(tokens.size(), 0.0);
    bool numeric_row = true;
    for (std::size_t i = 0; i < tokens.size(); ++i) {
      if (!TryParseDouble(tokens[i], numeric_values[i])) {
        numeric_row = false;
        break;
      }
    }

    if (!header_processed && !numeric_row) {
      data.x_label = tokens[0].empty() ? "Time" : tokens[0];
      for (std::size_t i = 1; i < tokens.size(); ++i) {
        if (!IsSupportedLoadSeriesColumn(tokens[i])) {
          continue;
        }
        selected_series_columns.push_back(i);
        data.series_names.push_back(tokens[i].empty() ? "Load " + std::to_string(i)
                                                      : tokens[i]);
      }
      if (selected_series_columns.empty()) {
        error_message = "No load columns matching f<number> or set<number> were found in the header.";
        return false;
      }
      header_processed = true;
      continue;
    }

    if (!numeric_row) {
      error_message = "Invalid numeric data in line " + std::to_string(line_number) + ".";
      data.time_values.clear();
      data.series_values.clear();
      return false;
    }

    if (!header_processed) {
      header_processed = true;
      data.series_names.resize(tokens.size() - 1);
      selected_series_columns.resize(tokens.size() - 1);
      for (std::size_t i = 0; i < data.series_names.size(); ++i) {
        selected_series_columns[i] = i + 1;
        data.series_names[i] = "Series " + std::to_string(i + 1);
      }
    }

    if (selected_series_columns.empty()) {
      error_message = "No series columns selected for plotting.";
      data.time_values.clear();
      data.series_values.clear();
      return false;
    }

    if (tokens.size() <= selected_series_columns.back()) {
      error_message = "Inconsistent column count in line " + std::to_string(line_number) + ".";
      data.time_values.clear();
      data.series_values.clear();
      return false;
    }

    if (data.series_values.empty()) {
      data.series_values.resize(selected_series_columns.size());
    }

    data.time_values.push_back(numeric_values[0]);
    for (std::size_t i = 0; i < selected_series_columns.size(); ++i) {
      data.series_values[i].push_back(numeric_values[selected_series_columns[i]]);
    }
  }

  if (data.time_values.empty()) {
    error_message = "No plot data found in CSV file: " + csv_path;
    return false;
  }

  return true;
}

std::string JoinPaths(const std::vector<std::string>& paths) {
  std::ostringstream oss;
  for (std::size_t i = 0; i < paths.size(); ++i) {
    if (i > 0) {
      oss << " | ";
    }
    oss << paths[i];
  }
  return oss.str();
}

std::string MakeUniqueSeriesName(const std::string& candidate,
                                 const std::vector<std::string>& existing) {
  if (std::find(existing.begin(), existing.end(), candidate) == existing.end()) {
    return candidate;
  }
  int suffix = 2;
  std::string unique_name;
  do {
    unique_name = candidate + " (" + std::to_string(suffix) + ")";
    ++suffix;
  } while (std::find(existing.begin(), existing.end(), unique_name) != existing.end());
  return unique_name;
}

}  // namespace

void LoadPlotDialog::SetCsvPath(const std::string& csv_path) {
  SetCsvPaths(csv_path.empty() ? std::vector<std::string>() : std::vector<std::string>{csv_path});
}

void LoadPlotDialog::SetCsvPaths(const std::vector<std::string>& csv_paths) {
  if (csv_paths == m_csv_paths && !m_time_values.empty()) {
    return;
  }
  LoadCsvFiles(csv_paths);
}

bool LoadPlotDialog::LoadCsv(const std::string& csv_path) {
  return LoadCsvFiles(csv_path.empty() ? std::vector<std::string>()
                                       : std::vector<std::string>{csv_path});
}

bool LoadPlotDialog::LoadCsvFiles(const std::vector<std::string>& csv_paths) {
  std::map<std::string, bool> previous_visibility;
  for (std::size_t i = 0; i < m_series_names.size() && i < m_series_visible.size(); ++i) {
    previous_visibility[m_series_names[i]] = m_series_visible[i];
  }

  m_csv_paths = csv_paths;
  m_csv_path = JoinPaths(csv_paths);
  m_error_message.clear();
  m_x_label = "Time";
  m_series_names.clear();
  m_series_visible.clear();
  m_time_values.clear();
  m_series_values.clear();
  m_auto_fit_pending = true;

  if (csv_paths.empty()) {
    m_error_message = "CSV path is empty.";
    return false;
  }

  bool loaded_any = false;
  std::vector<std::string> warnings;
  for (const std::string& csv_path : csv_paths) {
    if (csv_path.empty()) {
      continue;
    }

    LoadedCsvData loaded;
    std::string file_error;
    if (!LoadPlotCsvFile(csv_path, loaded, file_error)) {
      warnings.push_back(file_error);
      continue;
    }

    if (!loaded_any) {
      m_x_label = loaded.x_label;
      m_time_values = loaded.time_values;
      m_series_names = loaded.series_names;
      m_series_values = loaded.series_values;
      loaded_any = true;
      continue;
    }

    if (loaded.time_values.size() != m_time_values.size()) {
      warnings.push_back("Skipping CSV with different sample count: " + csv_path);
      continue;
    }

    bool same_time_grid = true;
    for (std::size_t i = 0; i < m_time_values.size(); ++i) {
      if (std::abs(loaded.time_values[i] - m_time_values[i]) > 1.0e-12) {
        same_time_grid = false;
        break;
      }
    }
    if (!same_time_grid) {
      warnings.push_back("Skipping CSV with different time grid: " + csv_path);
      continue;
    }

    for (std::size_t i = 0; i < loaded.series_names.size(); ++i) {
      m_series_names.push_back(MakeUniqueSeriesName(loaded.series_names[i], m_series_names));
      m_series_values.push_back(loaded.series_values[i]);
    }
  }

  if (!loaded_any || m_time_values.empty()) {
    m_error_message = warnings.empty() ? "No plot data found in CSV file." : warnings.front();
    return false;
  }

  m_series_visible.resize(m_series_names.size(), true);
  for (std::size_t i = 0; i < m_series_names.size(); ++i) {
    const auto it = previous_visibility.find(m_series_names[i]);
    m_series_visible[i] = (it != previous_visibility.end()) ? it->second : true;
  }

  return true;
}

void LoadPlotDialog::Draw(const char* title, bool* p_open) {
  if (p_open != nullptr && !*p_open) {
    return;
  }

  ImGui::SetNextWindowSize(ImVec2(900.0f, 520.0f), ImGuiCond_FirstUseEver);
  if (!ImGui::Begin(title, p_open)) {
    ImGui::End();
    return;
  }

  ImGui::TextWrapped("Source: %s", m_csv_path.empty() ? "<none>" : m_csv_path.c_str());
  if (!m_csv_paths.empty()) {
    unsigned long long total_file_size = 0;
    int existing_files = 0;
    for (const std::string& path : m_csv_paths) {
      if (!path.empty() && std::filesystem::exists(path)) {
        total_file_size += static_cast<unsigned long long>(std::filesystem::file_size(path));
        ++existing_files;
      }
    }
    ImGui::Text("Samples: %d", static_cast<int>(m_time_values.size()));
    ImGui::SameLine();
    ImGui::Text("Series: %d", static_cast<int>(m_series_names.size()));
    ImGui::SameLine();
    ImGui::Text("Files: %d  Size: %llu bytes", existing_files, total_file_size);
  }

  if (ImGui::Button("Reload")) {
    LoadCsvFiles(m_csv_paths);
  }
  ImGui::SameLine();
  if (ImGui::Button("Auto Scale")) {
    m_auto_fit_pending = true;
  }

  if (!m_error_message.empty()) {
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.45f, 1.0f), "%s", m_error_message.c_str());
  }

  if (!m_error_message.empty() || m_time_values.empty() || m_series_values.empty()) {
    ImGui::End();
    return;
  }

  ImGui::Spacing();
  int visible_series_count = 0;
  for (std::size_t i = 0; i < m_series_visible.size(); ++i) {
    if (m_series_visible[i]) {
      ++visible_series_count;
    }
  }

  std::string combo_label;
  if (visible_series_count == 0) {
    combo_label = "No series";
  } else if (visible_series_count == static_cast<int>(m_series_names.size())) {
    combo_label = "All series";
  } else if (visible_series_count == 1) {
    for (std::size_t i = 0; i < m_series_names.size(); ++i) {
      if (m_series_visible[i]) {
        combo_label = m_series_names[i];
        break;
      }
    }
  } else {
    combo_label = std::to_string(visible_series_count) + " series";
  }

  ImGui::SetNextItemWidth(260.0f);
  if (ImGui::BeginCombo("Visible Series", combo_label.c_str())) {
    if (ImGui::Selectable("Show All")) {
      std::fill(m_series_visible.begin(), m_series_visible.end(), true);
      m_auto_fit_pending = true;
    }
    if (ImGui::Selectable("Hide All")) {
      std::fill(m_series_visible.begin(), m_series_visible.end(), false);
      m_auto_fit_pending = true;
    }
    ImGui::Separator();

    for (std::size_t i = 0; i < m_series_names.size(); ++i) {
      bool visible = (i < m_series_visible.size()) ? m_series_visible[i] : true;
      if (ImGui::Checkbox(m_series_names[i].c_str(), &visible)) {
        if (i >= m_series_visible.size()) {
          m_series_visible.resize(m_series_names.size(), true);
        }
        m_series_visible[i] = visible;
        m_auto_fit_pending = true;
      }
    }
    ImGui::EndCombo();
  }

  if (visible_series_count == 0) {
    ImGui::Spacing();
    ImGui::TextDisabled("Select at least one series to plot.");
    ImGui::End();
    return;
  }

  if (m_auto_fit_pending) {
    ImPlot::SetNextAxesToFit();
    m_auto_fit_pending = false;
  }

  if (ImPlot::BeginPlot("##ContactForcesPlot", ImVec2(-1.0f, -1.0f))) {
    ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside);
    ImPlot::SetupAxes(m_x_label.c_str(), "Force");
    ImPlot::SetupAxisLimits(ImAxis_X1, m_time_values.front(), m_time_values.back(), ImGuiCond_Once);

    for (std::size_t i = 0; i < m_series_values.size(); ++i) {
      if (i < m_series_visible.size() && !m_series_visible[i]) {
        continue;
      }
      const char* label = m_series_names[i].c_str();
      ImPlot::PlotLine(label, m_time_values.data(), m_series_values[i].data(), static_cast<int>(m_time_values.size()));
    }

    ImPlot::EndPlot();
  }

  ImGui::End();
}
