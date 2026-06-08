#include "energy_plot_dialog.h"

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

std::string TrimEnergy(const std::string& value) {
  const std::string whitespace = " \t\r\n";
  const std::size_t begin = value.find_first_not_of(whitespace);
  if (begin == std::string::npos) {
    return "";
  }
  const std::size_t end = value.find_last_not_of(whitespace);
  return value.substr(begin, end - begin + 1);
}

std::vector<std::string> SplitEnergyCsvLine(const std::string& line) {
  std::vector<std::string> tokens;
  std::stringstream stream(line);
  std::string token;
  while (std::getline(stream, token, ',')) {
    tokens.push_back(TrimEnergy(token));
  }
  if (!line.empty() && line.back() == ',') {
    tokens.emplace_back();
  }
  return tokens;
}

bool TryParseEnergyDouble(const std::string& token, double& value) {
  const std::string trimmed = TrimEnergy(token);
  if (trimmed.empty()) {
    return false;
  }

  char* end_ptr = nullptr;
  errno = 0;
  const double parsed = std::strtod(trimmed.c_str(), &end_ptr);
  if (end_ptr == trimmed.c_str() || *end_ptr != '\0') {
    return false;
  }
  if (errno == ERANGE && !std::isfinite(parsed)) {
    return false;
  }

  value = parsed;
  return true;
}

std::string ToLowerEnergy(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}

bool IsDefaultVisibleEnergySeries(const std::string& name) {
  const std::string normalized = ToLowerEnergy(TrimEnergy(name));
  return normalized == "eint" ||
         normalized == "ekin" ||
         normalized == "wext" ||
         normalized == "etot" ||
         normalized == "ebal" ||
         normalized == "wnormal" ||
         normalized == "wfric";
}

}  // namespace

void EnergyPlotDialog::SetCsvPath(const std::string& csv_path) {
  if (csv_path == m_csv_path && !m_time_values.empty()) {
    return;
  }
  LoadCsv(csv_path);
}

bool EnergyPlotDialog::LoadCsv(const std::string& csv_path) {
  std::map<std::string, bool> previous_visibility;
  for (std::size_t i = 0; i < m_series_names.size() && i < m_series_visible.size(); ++i) {
    previous_visibility[m_series_names[i]] = m_series_visible[i];
  }

  m_csv_path = csv_path;
  m_error_message.clear();
  m_x_label = "Time";
  m_series_names.clear();
  m_series_visible.clear();
  m_time_values.clear();
  m_series_values.clear();
  m_auto_fit_pending = true;

  if (csv_path.empty()) {
    m_error_message = "CSV path is empty.";
    return false;
  }

  std::ifstream file(csv_path);
  if (!file.is_open()) {
    m_error_message = "Could not open CSV file: " + csv_path;
    return false;
  }

  std::string line;
  bool header_processed = false;
  int line_number = 0;
  std::vector<std::size_t> selected_series_columns;

  while (std::getline(file, line)) {
    ++line_number;
    const std::string trimmed_line = TrimEnergy(line);
    if (trimmed_line.empty()) {
      continue;
    }

    std::vector<std::string> tokens = SplitEnergyCsvLine(trimmed_line);
    if (tokens.size() < 2) {
      continue;
    }

    std::vector<double> numeric_values(tokens.size(), 0.0);
    bool numeric_row = true;
    for (std::size_t i = 0; i < tokens.size(); ++i) {
      if (!TryParseEnergyDouble(tokens[i], numeric_values[i])) {
        numeric_row = false;
        break;
      }
    }

    if (!header_processed && !numeric_row) {
      m_x_label = tokens[0].empty() ? "Time" : tokens[0];
      for (std::size_t i = 1; i < tokens.size(); ++i) {
        selected_series_columns.push_back(i);
        if (tokens[i].empty()) {
          m_series_names.push_back("Series " + std::to_string(i));
        } else {
          m_series_names.push_back(tokens[i]);
        }
      }
      if (selected_series_columns.empty()) {
        m_error_message = "No energy series columns were found in the header.";
        return false;
      }
      header_processed = true;
      continue;
    }

    if (!numeric_row) {
      m_error_message = "Invalid numeric data in line " + std::to_string(line_number) + ".";
      m_time_values.clear();
      m_series_values.clear();
      return false;
    }

    if (!header_processed) {
      header_processed = true;
      m_series_names.resize(tokens.size() - 1);
      selected_series_columns.resize(tokens.size() - 1);
      for (std::size_t i = 0; i < m_series_names.size(); ++i) {
        selected_series_columns[i] = i + 1;
        m_series_names[i] = "Series " + std::to_string(i + 1);
      }
    }

    if (tokens.size() <= selected_series_columns.back()) {
      m_error_message = "Inconsistent column count in line " + std::to_string(line_number) + ".";
      m_time_values.clear();
      m_series_values.clear();
      return false;
    }

    if (m_series_values.empty()) {
      m_series_values.resize(selected_series_columns.size());
    }

    m_time_values.push_back(numeric_values[0]);
    for (std::size_t i = 0; i < selected_series_columns.size(); ++i) {
      m_series_values[i].push_back(numeric_values[selected_series_columns[i]]);
    }
  }

  if (m_time_values.empty()) {
    m_error_message = "No plot data found in CSV file: " + csv_path;
    return false;
  }

  m_series_visible.resize(m_series_names.size(), true);
  for (std::size_t i = 0; i < m_series_names.size(); ++i) {
    const auto it = previous_visibility.find(m_series_names[i]);
    m_series_visible[i] =
        (it != previous_visibility.end()) ? it->second : IsDefaultVisibleEnergySeries(m_series_names[i]);
  }
  return true;
}

void EnergyPlotDialog::Draw(const char* title, bool* p_open) {
  if (p_open != nullptr && !*p_open) {
    return;
  }

  ImGui::SetNextWindowSize(ImVec2(900.0f, 520.0f), ImGuiCond_FirstUseEver);
  if (!ImGui::Begin(title, p_open)) {
    ImGui::End();
    return;
  }

  ImGui::TextWrapped("Source: %s", m_csv_path.empty() ? "<none>" : m_csv_path.c_str());
  if (!m_csv_path.empty() && std::filesystem::exists(m_csv_path)) {
    const auto file_size = std::filesystem::file_size(m_csv_path);
    ImGui::Text("Samples: %d", static_cast<int>(m_time_values.size()));
    ImGui::SameLine();
    ImGui::Text("Series: %d", static_cast<int>(m_series_names.size()));
    ImGui::SameLine();
    ImGui::Text("Size: %llu bytes", static_cast<unsigned long long>(file_size));
  }

  if (ImGui::Button("Reload")) {
    LoadCsv(m_csv_path);
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

  if (ImPlot::BeginPlot("##EnergyPlot", ImVec2(-1.0f, -1.0f))) {
    ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside);
    ImPlot::SetupAxes(m_x_label.c_str(), "Energy");
    ImPlot::SetupAxisLimits(ImAxis_X1, m_time_values.front(), m_time_values.back(), ImGuiCond_Once);

    for (std::size_t i = 0; i < m_series_values.size(); ++i) {
      if (i < m_series_visible.size() && !m_series_visible[i]) {
        continue;
      }
      ImPlot::PlotLine(m_series_names[i].c_str(),
                       m_time_values.data(),
                       m_series_values[i].data(),
                       static_cast<int>(m_time_values.size()));
    }

    ImPlot::EndPlot();
  }

  ImGui::End();
}
