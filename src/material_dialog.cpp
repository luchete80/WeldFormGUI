#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include "ImGuiFileDialog.h"

#include "material_dialog.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

using namespace std;

namespace {

constexpr const char *kTabulatedMaterialCsvDialogId = "ChooseFileDlgMaterialCsv";

constexpr int kMaterialCurveSamples = 256;

double ClampNonNegative(double value) {
  return value < 0.0 ? 0.0 : value;
}

double ClampPositive(double value, double fallback = 1.0e-6) {
  return value > 0.0 ? value : fallback;
}

double RangeMin(double a, double b) {
  return std::min(a, b);
}

double RangeMax(double a, double b) {
  return std::max(a, b);
}

double SafePlotMax(double value) {
  return value > 0.0 ? value : 1.0;
}

std::vector<double> BuildAxisRange(double min_value, double max_value) {
  double lo = min_value;
  double hi = max_value;
  if (hi <= lo) {
    hi = lo + 1.0;
  }

  std::vector<double> x;
  x.reserve(kMaterialCurveSamples + 1);
  const double delta = (hi - lo) / static_cast<double>(kMaterialCurveSamples);
  for (int i = 0; i <= kMaterialCurveSamples; ++i) {
    x.push_back(lo + delta * static_cast<double>(i));
  }
  return x;
}

std::vector<double> BuildJohnsonCookRates(const MaterialDialog &dialog) {
  const double ref_rate = ClampPositive(dialog.jc_eps0, 1.0);
  return {ref_rate * 0.1, ref_rate, ref_rate * 10.0};
}

std::vector<double> BuildGMTRates(const MaterialDialog &dialog) {
  double lo = ClampPositive(RangeMin(dialog.m_strain_rate_range_min, dialog.m_strain_rate_range_max), 1.0e-4);
  double hi = ClampPositive(RangeMax(dialog.m_strain_rate_range_min, dialog.m_strain_rate_range_max), lo * 10.0);
  if (hi < lo) {
    std::swap(lo, hi);
  }
  const double mid = std::sqrt(lo * hi);
  return {lo, mid, hi};
}

std::vector<std::string> MakeRateLabels(const std::vector<double> &rates) {
  std::vector<std::string> labels;
  labels.reserve(rates.size());
  for (double rate : rates) {
    std::ostringstream oss;
    oss << "edot=" << std::scientific << rate;
    labels.push_back(oss.str());
  }
  return labels;
}

std::string NormalizeHeader(std::string value) {
  std::string normalized;
  normalized.reserve(value.size());
  for (char c : value) {
    if (std::isalnum(static_cast<unsigned char>(c))) {
      normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
  }
  return normalized;
}

std::vector<std::string> SplitCsvLine(const std::string &line) {
  std::vector<std::string> tokens;
  std::string token;
  std::stringstream ss(line);
  while (std::getline(ss, token, ',')) {
    tokens.push_back(token);
  }
  return tokens;
}

bool ParseDouble(const std::string &token, double &value) {
  std::stringstream ss(token);
  ss >> value;
  return !ss.fail();
}

bool IsStrainHeader(const std::string &header) {
  return header == "strain" || header == "plasticstrain" || header == "equivalentstrain" ||
         header == "eps" || header == "epsp";
}

bool IsStrainRateHeader(const std::string &header) {
  return header == "strainrate" || header == "plasticstrainrate" || header == "equivalentstrainrate" ||
         header == "epsdot" || header == "edot";
}

bool IsTemperatureHeader(const std::string &header) {
  return header == "temperature" || header == "temp" || header == "t";
}

bool IsStressHeader(const std::string &header) {
  return header == "stress" || header == "flowstress" || header == "yieldstress" ||
         header == "sigma" || header == "sigmay";
}

bool LoadTabulatedCsv(const std::string &csv_path,
                      std::vector<double> &strain_grid,
                      std::vector<double> &rate_grid,
                      std::vector<double> &temperature_grid,
                      std::vector<double> &stress_values,
                      std::string &error) {
  std::ifstream file(csv_path);
  if (!file.is_open()) {
    error = "Could not open CSV file.";
    return false;
  }

  std::string header_line;
  if (!std::getline(file, header_line)) {
    error = "CSV file is empty.";
    return false;
  }

  const std::vector<std::string> headers = SplitCsvLine(header_line);
  int strain_col = -1;
  int rate_col = -1;
  int temp_col = -1;
  int stress_col = -1;
  for (int i = 0; i < static_cast<int>(headers.size()); ++i) {
    const std::string header = NormalizeHeader(headers[i]);
    if (IsStrainHeader(header)) strain_col = i;
    else if (IsStrainRateHeader(header)) rate_col = i;
    else if (IsTemperatureHeader(header)) temp_col = i;
    else if (IsStressHeader(header)) stress_col = i;
  }

  if (strain_col < 0 || rate_col < 0 || temp_col < 0 || stress_col < 0) {
    error = "Expected headers: strain, strain_rate, temperature, stress.";
    return false;
  }

  std::vector<double> strains;
  std::vector<double> rates;
  std::vector<double> temps;
  std::map<std::tuple<double, double, double>, double> stress_map;
  std::string line;
  while (std::getline(file, line)) {
    if (line.empty()) continue;
    const std::vector<std::string> tokens = SplitCsvLine(line);
    const int required_cols = std::max(std::max(strain_col, rate_col), std::max(temp_col, stress_col));
    if (static_cast<int>(tokens.size()) <= required_cols) {
      error = "Malformed CSV row.";
      return false;
    }
    double e = 0.0;
    double er = 0.0;
    double T = 0.0;
    double sigma = 0.0;
    if (!ParseDouble(tokens[strain_col], e) ||
        !ParseDouble(tokens[rate_col], er) ||
        !ParseDouble(tokens[temp_col], T) ||
        !ParseDouble(tokens[stress_col], sigma)) {
      error = "Invalid numeric value in CSV.";
      return false;
    }
    strains.push_back(e);
    rates.push_back(er);
    temps.push_back(T);
    stress_map[std::make_tuple(e, er, T)] = sigma;
  }

  std::sort(strains.begin(), strains.end());
  strains.erase(std::unique(strains.begin(), strains.end()), strains.end());
  std::sort(rates.begin(), rates.end());
  rates.erase(std::unique(rates.begin(), rates.end()), rates.end());
  std::sort(temps.begin(), temps.end());
  temps.erase(std::unique(temps.begin(), temps.end()), temps.end());

  if (strains.empty() || rates.empty() || temps.empty()) {
    error = "CSV table has no data.";
    return false;
  }

  strain_grid = strains;
  rate_grid = rates;
  temperature_grid = temps;
  stress_values.assign(strains.size() * rates.size() * temps.size(), std::numeric_limits<double>::quiet_NaN());

  for (size_t it = 0; it < temps.size(); ++it) {
    for (size_t ir = 0; ir < rates.size(); ++ir) {
      for (size_t ie = 0; ie < strains.size(); ++ie) {
        auto map_it = stress_map.find(std::make_tuple(strains[ie], rates[ir], temps[it]));
        if (map_it == stress_map.end()) {
          error = "CSV table is incomplete for the full strain/rate/temperature grid.";
          return false;
        }
        const size_t index = (it * rates.size() + ir) * strains.size() + ie;
        stress_values[index] = map_it->second;
      }
    }
  }

  return true;
}

void BuildRowsFromTabulatedTable(MaterialDialog &dialog) {
  dialog.m_tabulated_rows.clear();
  for (size_t it = 0; it < dialog.m_tabulated_temperature_grid.size(); ++it) {
    for (size_t ir = 0; ir < dialog.m_tabulated_rate_grid.size(); ++ir) {
      for (size_t ie = 0; ie < dialog.m_tabulated_strain_grid.size(); ++ie) {
        const size_t index =
            (it * dialog.m_tabulated_rate_grid.size() + ir) * dialog.m_tabulated_strain_grid.size() + ie;
        if (index >= dialog.m_tabulated_stress_values.size()) {
          continue;
        }
        MaterialDialog::TabulatedRow row;
        row.strain = dialog.m_tabulated_strain_grid[ie];
        row.strain_rate = dialog.m_tabulated_rate_grid[ir];
        row.temperature = dialog.m_tabulated_temperature_grid[it];
        row.stress = dialog.m_tabulated_stress_values[index];
        dialog.m_tabulated_rows.push_back(row);
      }
    }
  }
  dialog.m_tabulated_rows_dirty = false;
}

bool RebuildTabulatedTableFromRows(MaterialDialog &dialog) {
  dialog.m_tabulated_csv_error.clear();

  if (dialog.m_tabulated_rows.empty()) {
    dialog.m_tabulated_strain_grid.clear();
    dialog.m_tabulated_rate_grid.clear();
    dialog.m_tabulated_temperature_grid.clear();
    dialog.m_tabulated_stress_values.clear();
    dialog.m_tabulated_rows_dirty = false;
    return true;
  }

  std::vector<double> strains;
  std::vector<double> rates;
  std::vector<double> temps;
  std::map<std::tuple<double, double, double>, double> stress_map;

  for (const auto &row : dialog.m_tabulated_rows) {
    strains.push_back(row.strain);
    rates.push_back(row.strain_rate);
    temps.push_back(row.temperature);
    const auto key = std::make_tuple(row.strain, row.strain_rate, row.temperature);
    if (stress_map.find(key) != stress_map.end()) {
      dialog.m_tabulated_csv_error = "Duplicated strain/strain_rate/temperature row.";
      return false;
    }
    stress_map[key] = row.stress;
  }

  std::sort(strains.begin(), strains.end());
  strains.erase(std::unique(strains.begin(), strains.end()), strains.end());
  std::sort(rates.begin(), rates.end());
  rates.erase(std::unique(rates.begin(), rates.end()), rates.end());
  std::sort(temps.begin(), temps.end());
  temps.erase(std::unique(temps.begin(), temps.end()), temps.end());

  std::vector<double> stress_values(strains.size() * rates.size() * temps.size(),
                                    std::numeric_limits<double>::quiet_NaN());

  for (size_t it = 0; it < temps.size(); ++it) {
    for (size_t ir = 0; ir < rates.size(); ++ir) {
      for (size_t ie = 0; ie < strains.size(); ++ie) {
        const auto key = std::make_tuple(strains[ie], rates[ir], temps[it]);
        auto map_it = stress_map.find(key);
        if (map_it == stress_map.end()) {
          dialog.m_tabulated_csv_error =
              "Rows do not define a complete strain/rate/temperature Cartesian grid.";
          return false;
        }
        const size_t index = (it * rates.size() + ir) * strains.size() + ie;
        stress_values[index] = map_it->second;
      }
    }
  }

  dialog.m_tabulated_strain_grid = strains;
  dialog.m_tabulated_rate_grid = rates;
  dialog.m_tabulated_temperature_grid = temps;
  dialog.m_tabulated_stress_values = stress_values;
  dialog.m_strain_range_min = strains.front();
  dialog.m_strain_range_max = strains.back();
  dialog.m_strain_rate_range_min = rates.front();
  dialog.m_strain_rate_range_max = rates.back();
  dialog.m_temperature_range_min = temps.front();
  dialog.m_temperature_range_max = temps.back();
  dialog.m_tabulated_rows_dirty = false;
  return true;
}

bool ImportTabulatedCsvIntoDialog(const std::string &csv_path, MaterialDialog &dialog) {
  dialog.m_tabulated_csv_error.clear();
  if (!LoadTabulatedCsv(csv_path,
                        dialog.m_tabulated_strain_grid,
                        dialog.m_tabulated_rate_grid,
                        dialog.m_tabulated_temperature_grid,
                        dialog.m_tabulated_stress_values,
                        dialog.m_tabulated_csv_error)) {
    if (dialog.m_tabulated_csv_error.empty()) {
      dialog.m_tabulated_csv_error = "CSV import failed.";
    }
    return false;
  }

  std::snprintf(dialog.m_tabulated_csv_path.data(),
                dialog.m_tabulated_csv_path.size(),
                "%s",
                csv_path.c_str());
  dialog.m_strain_range_min = dialog.m_tabulated_strain_grid.front();
  dialog.m_strain_range_max = dialog.m_tabulated_strain_grid.back();
  dialog.m_strain_rate_range_min = dialog.m_tabulated_rate_grid.front();
  dialog.m_strain_rate_range_max = dialog.m_tabulated_rate_grid.back();
  dialog.m_temperature_range_min = dialog.m_tabulated_temperature_grid.front();
  dialog.m_temperature_range_max = dialog.m_tabulated_temperature_grid.back();
  dialog.m_plot_temperature = dialog.m_temperature_range_min;
  dialog.m_plot_strain = 0.5 * (dialog.m_strain_range_min + dialog.m_strain_range_max);
  BuildRowsFromTabulatedTable(dialog);
  return true;
}

bool BuildHollomonCurve(const MaterialDialog &dialog,
                        std::vector<double> &strain_values,
                        std::vector<double> &stress_values,
                        double &eps0,
                        double &eps1,
                        double &eps_max,
                        double &sigma_max) {
  strain_values.clear();
  stress_values.clear();

  const double elastic_modulus = dialog.m_elastic_const;
  const double yield_stress0 = dialog.m_yield_stress0;
  const double K = dialog.hollomon_K;
  const double n = dialog.hollomon_n;

  eps0 = 0.0;
  eps1 = 0.0;
  eps_max = RangeMax(dialog.m_strain_range_min, dialog.m_strain_range_max);
  sigma_max = 0.0;

  if (elastic_modulus <= 0.0 || yield_stress0 <= 0.0 || K <= 0.0 || n <= 0.0) {
    return false;
  }

  Material_ preview_material;
  preview_material.strRange = {
    ClampNonNegative(RangeMin(dialog.m_strain_range_min, dialog.m_strain_range_max)),
    RangeMax(dialog.m_strain_range_min, dialog.m_strain_range_max)
  };
  preview_material.InitHollomon(Elastic_(elastic_modulus, dialog.m_poisson_const), yield_stress0, K, n);

  eps0 = preview_material.eps0;
  eps1 = preview_material.eps1;
  eps_max = std::max(preview_material.e_max, eps0);
  eps_max = std::max(eps_max, eps1);

  sigma_max = K * std::pow(std::max(eps_max, 1.0e-9), n);
  if (sigma_max < yield_stress0) {
    sigma_max = yield_stress0;
  }

  const double strain_start = ClampNonNegative(RangeMin(dialog.m_strain_range_min, dialog.m_strain_range_max));
  const double strain_end = std::max(eps_max, strain_start + 1.0e-9);
  const double delta = (strain_end - strain_start) / static_cast<double>(kMaterialCurveSamples);

  strain_values.reserve(kMaterialCurveSamples + 1);
  stress_values.reserve(kMaterialCurveSamples + 1);

  for (int i = 0; i <= kMaterialCurveSamples; ++i) {
    const double strain = strain_start + delta * static_cast<double>(i);
    double stress = 0.0;

    if (strain <= eps0) {
      stress = elastic_modulus * strain;
    } else if (strain <= eps1) {
      stress = yield_stress0;
    } else {
      stress = K * std::pow(strain, n);
    }

    strain_values.push_back(strain);
    stress_values.push_back(stress);
  }

  return true;
}

template <typename StressFn>
bool BuildFamilyCurves(const std::vector<double> &x_values,
                       const std::vector<double> &rate_values,
                       StressFn stress_fn,
                       std::vector<std::vector<double>> &family_curves,
                       double &y_max) {
  family_curves.clear();
  y_max = 0.0;

  if (x_values.empty() || rate_values.empty()) {
    return false;
  }

  family_curves.resize(rate_values.size());
  for (size_t curve_index = 0; curve_index < rate_values.size(); ++curve_index) {
    auto &curve = family_curves[curve_index];
    curve.reserve(x_values.size());

    for (double x : x_values) {
      const double stress = stress_fn(x, rate_values[curve_index]);
      curve.push_back(stress);
      y_max = std::max(y_max, stress);
    }
  }

  return y_max > 0.0;
}

void DrawPlotToolbar(const char *reset_id, bool &fit_next) {
  if (ImGui::Button(reset_id)) {
    fit_next = true;
  }
  ImGui::SameLine();
  ImGui::TextDisabled("Wheel: zoom | Drag: pan");
}

void DrawFamilyPlot(const char *plot_id, const char *x_label, const char *y_label,
                    const std::vector<double> &x_values,
                    const std::vector<std::vector<double>> &family_curves,
                    const std::vector<std::string> &labels,
                    double y_max,
                    bool &fit_next) {
  if (fit_next) {
    ImPlot::SetNextAxesToFit();
  }

  if (ImPlot::BeginPlot(plot_id, ImVec2(-1.0f, 260.0f))) {
    ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside);
    ImPlot::SetupAxes(x_label, y_label);
    ImPlot::SetupAxisLimits(ImAxis_X1, x_values.front(), x_values.back(), ImGuiCond_Once);
    ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, SafePlotMax(y_max) * 1.05, ImGuiCond_Once);

    for (size_t i = 0; i < family_curves.size(); ++i) {
      ImPlot::PlotLine(labels[i].c_str(), x_values.data(), family_curves[i].data(),
                       static_cast<int>(x_values.size()));
    }

    if (ImPlot::IsPlotHovered()) {
      const ImPlotPoint mouse_pos = ImPlot::GetPlotMousePos();
      ImGui::BeginTooltip();
      ImGui::Text("%s: %.6e", x_label, mouse_pos.x);
      ImGui::Text("%s: %.6e", y_label, mouse_pos.y);
      ImGui::EndTooltip();
    }

    fit_next = false;
    ImPlot::EndPlot();
  }
}

void DrawHollomonPlot(const MaterialDialog &dialog) {
  static bool fit_next = true;

  std::vector<double> strain_values;
  std::vector<double> stress_values;
  double eps0 = 0.0;
  double eps1 = 0.0;
  double eps_max = 0.0;
  double sigma_max = 0.0;

  if (!BuildHollomonCurve(dialog, strain_values, stress_values, eps0, eps1, eps_max, sigma_max)) {
    ImGui::TextDisabled("Set positive E, sigma_y0, K, and n values to plot.");
    return;
  }

  ImGui::Text("eps0 = %.4e   eps1 = %.4e   eps_max = %.4e", eps0, eps1, eps_max);
  if (eps0 > eps1) {
    ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.45f, 1.0f),
                       "Warning: eps0 > eps1. Check sigma_y0, E, K, or n.");
  }
  ImGui::TextDisabled("Hollomon does not depend on strain rate or temperature.");
  DrawPlotToolbar("Reset Zoom##hollomon", fit_next);

  if (fit_next) {
    ImPlot::SetNextAxesToFit();
  }

  if (ImPlot::BeginPlot("##HollomonCurve", ImVec2(-1.0f, 260.0f))) {
    ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside);
    ImPlot::SetupAxes("Strain", "Stress");
    ImPlot::SetupAxisLimits(ImAxis_X1, strain_values.front(), strain_values.back(), ImGuiCond_Once);
    ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, SafePlotMax(sigma_max) * 1.05, ImGuiCond_Once);
    ImPlot::PlotLine("Hollomon", strain_values.data(), stress_values.data(),
                     static_cast<int>(strain_values.size()));

    const double markers_x[] = {eps0, eps1, eps_max};
    const double markers_y[] = {dialog.m_yield_stress0, dialog.m_yield_stress0, sigma_max};
    ImPlot::PlotScatter("Key points", markers_x, markers_y, 3,
                        ImPlotSpec(ImPlotProp_Marker, ImPlotMarker_Circle,
                                   ImPlotProp_MarkerSize, 4.0f));

    fit_next = false;
    ImPlot::EndPlot();
  }
}

bool BuildJohnsonCookPreviewMaterial(const MaterialDialog &dialog, Material_ &preview_material) {
  if (dialog.m_elastic_const <= 0.0 || dialog.m_yield_stress0 <= 0.0 ||
      dialog.jc_B < 0.0 || dialog.jc_n < 0.0 || dialog.jc_eps0 <= 0.0 ||
      dialog.jc_m < 0.0) {
    return false;
  }

  preview_material.strRange = {
    ClampNonNegative(RangeMin(dialog.m_strain_range_min, dialog.m_strain_range_max)),
    RangeMax(dialog.m_strain_range_min, dialog.m_strain_range_max)
  };
  preview_material.e_min = preview_material.strRange[0];
  preview_material.e_max = preview_material.strRange[1];
  preview_material.Init_JohnsonCook(Elastic_(dialog.m_elastic_const, dialog.m_poisson_const),
                                    dialog.m_yield_stress0, dialog.jc_B, dialog.jc_n,
                                    dialog.jc_C, dialog.jc_eps0, dialog.jc_m,
                                    dialog.jc_Tm, dialog.jc_Tt);
  return true;
}

void DrawJohnsonCookPlots(const MaterialDialog &dialog) {
  static bool fit_strain_next = true;
  static bool fit_temp_next = true;

  Material_ preview_material;
  if (!BuildJohnsonCookPreviewMaterial(dialog, preview_material)) {
    ImGui::TextDisabled("Set E, sigma_y0, B, n, C, eps0, and m to plot.");
    return;
  }

  const double strain_min = ClampNonNegative(RangeMin(dialog.m_strain_range_min, dialog.m_strain_range_max));
  const double strain_max = std::max(RangeMax(dialog.m_strain_range_min, dialog.m_strain_range_max),
                                     strain_min + 1.0e-6);
  const double temp_min = std::min(dialog.jc_Tt, dialog.jc_Tm);
  const double temp_max = std::max(dialog.jc_Tt, dialog.jc_Tm);
  const double plot_temp = ClampMaterialValue(dialog.m_plot_temperature, temp_min, std::max(temp_max, temp_min + 1.0));
  const double plot_strain = ClampMaterialValue(dialog.m_plot_strain, strain_min, strain_max);

  const std::vector<double> rate_values = BuildJohnsonCookRates(dialog);
  const std::vector<std::string> rate_labels = MakeRateLabels(rate_values);

  const std::vector<double> strain_values = BuildAxisRange(strain_min, strain_max);
  std::vector<std::vector<double>> strain_family_curves;
  double strain_y_max = 0.0;
  BuildFamilyCurves(
    strain_values, rate_values,
    [&preview_material, plot_temp](double strain, double strain_rate) {
      return CalcJohnsonCookYieldStress(strain, strain_rate, plot_temp, &preview_material);
    },
    strain_family_curves, strain_y_max);

  ImGui::Text("Stress-strain curves at T = %.3e", plot_temp);
  DrawPlotToolbar("Reset Zoom##jc_strain", fit_strain_next);
  DrawFamilyPlot("##JohnsonCookCurve", "Strain", "Stress", strain_values, strain_family_curves,
                 rate_labels, strain_y_max, fit_strain_next);

  const std::vector<double> temperature_values = BuildAxisRange(temp_min, std::max(temp_max, temp_min + 1.0));
  std::vector<std::vector<double>> temp_family_curves;
  double temp_y_max = 0.0;
  BuildFamilyCurves(
    temperature_values, rate_values,
    [&preview_material, plot_strain](double temp, double strain_rate) {
      return CalcJohnsonCookYieldStress(plot_strain, strain_rate, temp, &preview_material);
    },
    temp_family_curves, temp_y_max);

  ImGui::Text("Stress-temperature curves at strain = %.3e", plot_strain);
  DrawPlotToolbar("Reset Zoom##jc_temp", fit_temp_next);
  DrawFamilyPlot("##JohnsonCookTempCurve", "Temperature", "Stress", temperature_values,
                 temp_family_curves, rate_labels, temp_y_max, fit_temp_next);
}

bool BuildGMTPreviewMaterial(const MaterialDialog &dialog, Material_ &preview_material) {
  if (dialog.m_elastic_const <= 0.0 || dialog.gmt_C1 == 0.0) {
    return false;
  }

  const double strain_min = ClampPositive(ClampNonNegative(RangeMin(dialog.m_strain_range_min, dialog.m_strain_range_max)));
  const double strain_max = std::max(RangeMax(dialog.m_strain_range_min, dialog.m_strain_range_max),
                                     strain_min + 1.0e-6);
  const double rate_min = ClampPositive(RangeMin(dialog.m_strain_rate_range_min, dialog.m_strain_rate_range_max), 1.0e-6);
  const double rate_max = std::max(ClampPositive(RangeMax(dialog.m_strain_rate_range_min, dialog.m_strain_rate_range_max), rate_min),
                                   rate_min);
  const double temp_min = RangeMin(dialog.m_temperature_range_min, dialog.m_temperature_range_max);
  const double temp_max = std::max(RangeMax(dialog.m_temperature_range_min, dialog.m_temperature_range_max),
                                   temp_min + 1.0);

  preview_material.Init_GMT(Elastic_(dialog.m_elastic_const, dialog.m_poisson_const),
                            dialog.gmt_n1, dialog.gmt_n2, dialog.gmt_C1, dialog.gmt_C2,
                            dialog.gmt_m1, dialog.gmt_m2, dialog.gmt_I1, dialog.gmt_I2,
                            strain_min, strain_max, rate_min, rate_max, temp_min, temp_max);
  preview_material.yieldStress0 = dialog.m_yield_stress0;
  return true;
}

void DrawGMTPlots(const MaterialDialog &dialog) {
  static bool fit_strain_next = true;
  static bool fit_temp_next = true;

  Material_ preview_material;
  if (!BuildGMTPreviewMaterial(dialog, preview_material)) {
    ImGui::TextDisabled("Set at least E and C1 to plot GMT.");
    return;
  }

  const std::vector<double> rate_values = BuildGMTRates(dialog);
  const std::vector<std::string> rate_labels = MakeRateLabels(rate_values);

  const double strain_min = preview_material.e_min;
  const double strain_max = preview_material.e_max;
  const double temp_min = preview_material.T_min;
  const double temp_max = preview_material.T_max;
  const double plot_temp = ClampMaterialValue(dialog.m_plot_temperature, temp_min, temp_max);
  const double plot_strain = ClampMaterialValue(dialog.m_plot_strain, strain_min, strain_max);

  const std::vector<double> strain_values = BuildAxisRange(strain_min, strain_max);
  std::vector<std::vector<double>> strain_family_curves;
  double strain_y_max = 0.0;
  BuildFamilyCurves(
    strain_values, rate_values,
    [&preview_material, plot_temp](double strain, double strain_rate) {
      return CalcGMTYieldStress(strain, strain_rate, plot_temp, &preview_material);
    },
    strain_family_curves, strain_y_max);

  ImGui::Text("Stress-strain curves at T = %.3e", plot_temp);
  DrawPlotToolbar("Reset Zoom##gmt_strain", fit_strain_next);
  DrawFamilyPlot("##GMTCurve", "Strain", "Stress", strain_values, strain_family_curves,
                 rate_labels, strain_y_max, fit_strain_next);

  const std::vector<double> temperature_values = BuildAxisRange(temp_min, temp_max);
  std::vector<std::vector<double>> temp_family_curves;
  double temp_y_max = 0.0;
  BuildFamilyCurves(
    temperature_values, rate_values,
    [&preview_material, plot_strain](double temp, double strain_rate) {
      return CalcGMTYieldStress(plot_strain, strain_rate, temp, &preview_material);
    },
    temp_family_curves, temp_y_max);

  ImGui::Text("Stress-temperature curves at strain = %.3e", plot_strain);
  DrawPlotToolbar("Reset Zoom##gmt_temp", fit_temp_next);
  DrawFamilyPlot("##GMTTempCurve", "Temperature", "Stress", temperature_values,
                 temp_family_curves, rate_labels, temp_y_max, fit_temp_next);
}

bool BuildTabulatedPreviewMaterial(const MaterialDialog &dialog, Material_ &preview_material) {
  if (dialog.m_tabulated_strain_grid.empty() || dialog.m_tabulated_rate_grid.empty() ||
      dialog.m_tabulated_temperature_grid.empty() || dialog.m_tabulated_stress_values.empty()) {
    return false;
  }

  preview_material.elastic_m = Elastic_(dialog.m_elastic_const, dialog.m_poisson_const);
  preview_material.Material_model = TABULATED;
  preview_material.tabulated_enabled = true;
  preview_material.tabulatedStrainGrid = dialog.m_tabulated_strain_grid;
  preview_material.tabulatedRateGrid = dialog.m_tabulated_rate_grid;
  preview_material.tabulatedTemperatureGrid = dialog.m_tabulated_temperature_grid;
  preview_material.tabulatedStressValues = dialog.m_tabulated_stress_values;
  preview_material.e_min = dialog.m_tabulated_strain_grid.front();
  preview_material.e_max = dialog.m_tabulated_strain_grid.back();
  preview_material.er_min = dialog.m_tabulated_rate_grid.front();
  preview_material.er_max = dialog.m_tabulated_rate_grid.back();
  preview_material.T_min = dialog.m_tabulated_temperature_grid.front();
  preview_material.T_max = dialog.m_tabulated_temperature_grid.back();
  preview_material.yieldStress0 = dialog.m_yield_stress0;
  return true;
}

void DrawTabulatedPlots(const MaterialDialog &dialog) {
  static bool fit_strain_next = true;
  static bool fit_temp_next = true;

  Material_ preview_material;
  if (!BuildTabulatedPreviewMaterial(dialog, preview_material)) {
    ImGui::TextDisabled("Import a CSV or load a tabulated family to plot.");
    return;
  }

  const std::vector<double> rate_values = preview_material.tabulatedRateGrid;
  const std::vector<std::string> rate_labels = MakeRateLabels(rate_values);
  const double strain_min = preview_material.e_min;
  const double strain_max = preview_material.e_max;
  const double temp_min = preview_material.T_min;
  const double temp_max = preview_material.T_max;
  const double plot_temp = ClampMaterialValue(dialog.m_plot_temperature, temp_min, temp_max);
  const double plot_strain = ClampMaterialValue(dialog.m_plot_strain, strain_min, strain_max);

  const std::vector<double> strain_values = BuildAxisRange(strain_min, strain_max);
  std::vector<std::vector<double>> strain_family_curves;
  double strain_y_max = 0.0;
  BuildFamilyCurves(
    strain_values, rate_values,
    [&preview_material, plot_temp](double strain, double strain_rate) {
      return CalcTabulatedYieldStress(strain, strain_rate, plot_temp, &preview_material);
    },
    strain_family_curves, strain_y_max);

  ImGui::Text("Stress-strain curves at T = %.3e", plot_temp);
  DrawPlotToolbar("Reset Zoom##tabulated_strain", fit_strain_next);
  DrawFamilyPlot("##TabulatedCurve", "Strain", "Stress", strain_values, strain_family_curves,
                 rate_labels, strain_y_max, fit_strain_next);

  const std::vector<double> temperature_values = BuildAxisRange(temp_min, temp_max);
  std::vector<std::vector<double>> temp_family_curves;
  double temp_y_max = 0.0;
  BuildFamilyCurves(
    temperature_values, rate_values,
    [&preview_material, plot_strain](double temp, double strain_rate) {
      return CalcTabulatedYieldStress(plot_strain, strain_rate, temp, &preview_material);
    },
    temp_family_curves, temp_y_max);

  ImGui::Text("Stress-temperature curves at strain = %.3e", plot_strain);
  DrawPlotToolbar("Reset Zoom##tabulated_temp", fit_temp_next);
  DrawFamilyPlot("##TabulatedTempCurve", "Temperature", "Stress", temperature_values,
                 temp_family_curves, rate_labels, temp_y_max, fit_temp_next);
}

}  // namespace

void MaterialDialog::InitFromMaterial(Material_* mat) {
  if (m_pl != nullptr) {
    delete m_pl;
    m_pl = nullptr;
  }

  if (!mat) {
    m_density_const = 1.0;
    m_elastic_const = 2.1e11;
    m_poisson_const = 0.3;
    m_selected_model = NONE;
    m_pl_const.clear();
    bilinear_sy0 = 0.0;
    bilinear_Et = 0.0;
    hollomon_K = 0.0;
    hollomon_n = 0.0;
    jc_B = 0.0;
    jc_n = 0.0;
    jc_C = 0.0;
    jc_eps0 = 1.0;
    jc_m = 1.0;
    jc_Tm = 1000.0;
    jc_Tt = 20.0;
    gmt_C1 = 0.0;
    gmt_C2 = 0.0;
    gmt_n1 = 0.0;
    gmt_n2 = 0.0;
    gmt_m1 = 0.0;
    gmt_m2 = 0.0;
    gmt_I1 = 0.0;
    gmt_I2 = 0.0;
    m_k_T = 0.0;
    m_cp_T = 0.0;
    m_yield_stress0 = 190.0E6;
    m_strain_range_min = 0.0;
    m_strain_range_max = 0.65;
    m_strain_rate_range_min = 1.0e-3;
    m_strain_rate_range_max = 1.0e1;
    m_temperature_range_min = 20.0;
    m_temperature_range_max = 1000.0;
    m_plot_temperature = 20.0;
    m_plot_strain = 0.2;
    m_tabulated_csv_path[0] = '\0';
    m_tabulated_csv_error.clear();
    m_tabulated_export_csv_reference = false;
    m_tabulated_strain_grid.clear();
    m_tabulated_rate_grid.clear();
    m_tabulated_temperature_grid.clear();
    m_tabulated_stress_values.clear();
    m_tabulated_rows.clear();
    m_tabulated_rows_dirty = false;
    return;
  }

  m_density_const = mat->getDensityConstant();
  m_elastic_const = mat->Elastic().E();
  m_poisson_const = mat->Elastic().nu();
  m_cp_T = mat->cp_T;
  m_k_T = mat->k_T;
  m_yield_stress0 = mat->yieldStress0;
  m_strain_range_min = 0.0;
  m_strain_range_max = 0.65;
  if (mat->strRange.size() >= 2) {
    m_strain_range_min = mat->strRange[0];
    m_strain_range_max = mat->strRange[1];
  }
  m_strain_rate_range_min = mat->er_min > 0.0 ? mat->er_min : 1.0e-3;
  m_strain_rate_range_max = mat->er_max > 0.0 ? mat->er_max : 1.0e1;
  m_temperature_range_min = mat->T_min;
  m_temperature_range_max = mat->T_max > mat->T_min ? mat->T_max : std::max(mat->T_m, 1000.0);
  if (m_temperature_range_max <= m_temperature_range_min) {
    m_temperature_range_min = mat->T_t;
    m_temperature_range_max = std::max(mat->T_m, mat->T_t + 1.0);
  }
  m_plot_temperature = m_temperature_range_min;
  m_plot_strain = 0.5 * (m_strain_range_min + m_strain_range_max);

  bilinear_sy0 = 0.0;
  bilinear_Et = 0.0;
  hollomon_K = mat->K;
  hollomon_n = mat->m;
  jc_B = mat->B;
  jc_n = mat->n;
  jc_C = mat->C;
  jc_eps0 = mat->eps_0 > 0.0 ? mat->eps_0 : 1.0;
  jc_m = mat->m;
  jc_Tm = mat->T_m;
  jc_Tt = mat->T_t;
  gmt_C1 = mat->C1;
  gmt_C2 = mat->C2;
  gmt_n1 = mat->n1;
  gmt_n2 = mat->n2;
  gmt_m1 = mat->m1;
  gmt_m2 = mat->m2;
  gmt_I1 = mat->I1;
  gmt_I2 = mat->I2;
  std::snprintf(m_tabulated_csv_path.data(), m_tabulated_csv_path.size(), "%s", mat->tableCsvPath.c_str());
  m_tabulated_csv_error.clear();
  m_tabulated_export_csv_reference = mat->tabulated_export_csv_reference;
  m_tabulated_strain_grid = mat->tabulatedStrainGrid;
  m_tabulated_rate_grid = mat->tabulatedRateGrid;
  m_tabulated_temperature_grid = mat->tabulatedTemperatureGrid;
  m_tabulated_stress_values = mat->tabulatedStressValues;
  BuildRowsFromTabulatedTable(*this);

  if (mat->isPlastic()) {
    m_pl = mat->getPlastic()->clone();
    m_selected_model = m_pl->getType();
    m_pl_const = m_pl->getPlasticConstants();

    switch (m_selected_model) {
      case BILINEAR:
        if (m_pl_const.size() >= 2) {
          bilinear_sy0 = m_pl_const[0];
          bilinear_Et = m_pl_const[1];
        }
        break;

      case HOLLOMON:
        if (m_pl_const.size() >= 2) {
          hollomon_K = m_pl_const[0];
          hollomon_n = m_pl_const[1];
        }
        break;

      case JOHNSON_COOK:
        if (m_pl_const.size() >= 7) {
          jc_B = m_pl_const[0];
          jc_n = m_pl_const[1];
          jc_C = m_pl_const[2];
          jc_eps0 = m_pl_const[3];
          jc_m = m_pl_const[4];
          jc_Tm = m_pl_const[5];
          jc_Tt = m_pl_const[6];
        }
        break;

      case _GMT_:
        if (m_pl_const.size() >= 8) {
          gmt_n1 = m_pl_const[0];
          gmt_n2 = m_pl_const[1];
          gmt_C1 = m_pl_const[2];
          gmt_C2 = m_pl_const[3];
          gmt_m1 = m_pl_const[4];
          gmt_m2 = m_pl_const[5];
          gmt_I1 = m_pl_const[6];
          gmt_I2 = m_pl_const[7];
        }
        break;

      case TABULATED:
        break;

      default:
        break;
    }
  } else {
    m_selected_model = NONE;
  }
}

void MaterialDialog::Draw(const char* title, bool* p_open, Material_ *mat, Material_Db *mat_db) {
  static int item_current = 0;

  create_material = false;
  cancel_action = false;
  if (!m_initiated) {
    InitFromMaterial(mat);
    item_current = m_selected_model;
    m_initiated = true;
  }

  if (!ImGui::Begin(title, p_open)) {
    ImGui::End();
    return;
  }

  ImGui::InputDouble("Density", &m_density_const, 0.0f, 1.0f, "%.4f");
  ImGui::InputDouble("Elastic Mod", &m_elastic_const, 0.0f, 1.0f, "%.2e");
  ImGui::InputDouble("Poisson Mod", &m_poisson_const, 0.0f, 1.0f, "%.4f");
  ImGui::InputDouble("Initial Yield Stress", &m_yield_stress0, 0.0f, 1.0f, "%.2e");
  ImGui::InputDouble("Strain Range Min", &m_strain_range_min, 0.0f, 1.0f, "%.4f");
  ImGui::InputDouble("Strain Range Max", &m_strain_range_max, 0.0f, 1.0f, "%.4f");

  if (m_thermal_coupling) {
    ImGui::InputDouble("Heat Capacity (cp)", &m_cp_T, 0.0, 1.0, "%.2f");
    ImGui::InputDouble("Thermal Conductivity (k)", &m_k_T, 0.0, 1.0, "%.2f");
  } else {
    ImGui::TextDisabled("Thermal coupling disabled in model: cp/k are hidden.");
  }

  const char* items[] = { "None", "Bilinear", "Hollomon", "Johnson Cook", "GMT", "Sinh", "Tabulated" };

  ImGui::SetNextItemOpen(true, ImGuiCond_Once);
  if (ImGui::CollapsingHeader("Plastic")) {
    ImGui::Combo("Yield Criteria", &item_current, items, IM_ARRAYSIZE(items));

    if (item_current == BILINEAR) {
      ImGui::InputDouble("Yield Stress", &bilinear_sy0, 0.0f, 1.0f, "%.4e");
      ImGui::InputDouble("Tangent Mod", &bilinear_Et, 0.0f, 1.0f, "%.4e");
    }

    if (item_current == HOLLOMON) {
      ImGui::InputDouble("K", &hollomon_K, 0.0f, 1.0f, "%.4e");
      ImGui::InputDouble("n", &hollomon_n, 0.0f, 1.0f, "%.4f");
      ImGui::Spacing();
      DrawHollomonPlot(*this);
    }

    if (item_current == JOHNSON_COOK) {
      ImGui::InputDouble("B", &jc_B, 0.0f, 1.0f, "%.4e");
      ImGui::InputDouble("n", &jc_n, 0.0f, 1.0f, "%.4f");
      ImGui::InputDouble("C", &jc_C, 0.0f, 1.0f, "%.4f");
      ImGui::InputDouble("eps0", &jc_eps0, 0.0f, 1.0f, "%.4e");
      ImGui::InputDouble("m", &jc_m, 0.0f, 1.0f, "%.4f");
      ImGui::InputDouble("T melt", &jc_Tm, 0.0f, 1.0f, "%.4e");
      ImGui::InputDouble("T transition", &jc_Tt, 0.0f, 1.0f, "%.4e");
      ImGui::InputDouble("Plot Temperature", &m_plot_temperature, 0.0f, 1.0f, "%.4e");
      ImGui::InputDouble("Plot Strain", &m_plot_strain, 0.0f, 1.0f, "%.4f");
      ImGui::Spacing();
      DrawJohnsonCookPlots(*this);
    }

    if (item_current == _GMT_) {
      ImGui::InputDouble("n1", &gmt_n1, 0.0f, 1.0f, "%.4f");
      ImGui::InputDouble("n2", &gmt_n2, 0.0f, 1.0f, "%.4f");
      ImGui::InputDouble("C1", &gmt_C1, 0.0f, 1.0f, "%.4e");
      ImGui::InputDouble("C2", &gmt_C2, 0.0f, 1.0f, "%.4e");
      ImGui::InputDouble("m1", &gmt_m1, 0.0f, 1.0f, "%.4f");
      ImGui::InputDouble("m2", &gmt_m2, 0.0f, 1.0f, "%.4f");
      ImGui::InputDouble("I1", &gmt_I1, 0.0f, 1.0f, "%.4e");
      ImGui::InputDouble("I2", &gmt_I2, 0.0f, 1.0f, "%.4e");
      ImGui::InputDouble("Strain Rate Min", &m_strain_rate_range_min, 0.0f, 1.0f, "%.4e");
      ImGui::InputDouble("Strain Rate Max", &m_strain_rate_range_max, 0.0f, 1.0f, "%.4e");
      ImGui::InputDouble("Temperature Min", &m_temperature_range_min, 0.0f, 1.0f, "%.4e");
      ImGui::InputDouble("Temperature Max", &m_temperature_range_max, 0.0f, 1.0f, "%.4e");
      ImGui::InputDouble("Plot Temperature", &m_plot_temperature, 0.0f, 1.0f, "%.4e");
      ImGui::InputDouble("Plot Strain", &m_plot_strain, 0.0f, 1.0f, "%.4f");
      ImGui::Spacing();
      DrawGMTPlots(*this);
    }

    if (item_current == TABULATED) {
      ImGui::InputText("CSV Path", m_tabulated_csv_path.data(), m_tabulated_csv_path.size());
      ImGui::SameLine();
      if (ImGui::Button("Browse...")) {
        ImGuiFileDialog::Instance()->OpenDialog(kTabulatedMaterialCsvDialogId,
                                                "Choose CSV File",
                                                ".csv,.CSV",
                                                ".");
      }
      ImGui::SameLine();
      if (ImGui::Button("Import CSV")) {
        ImportTabulatedCsvIntoDialog(m_tabulated_csv_path.data(), *this);
      }
      ImGui::SameLine();
      if (ImGui::Button("Add Row")) {
        MaterialDialog::TabulatedRow row;
        if (!m_tabulated_rows.empty()) {
          row = m_tabulated_rows.back();
        } else {
          row.stress = m_yield_stress0;
        }
        m_tabulated_rows.push_back(row);
        m_tabulated_rows_dirty = true;
      }
      ImGui::SameLine();
      if (ImGui::Button("Duplicate Last") && !m_tabulated_rows.empty()) {
        m_tabulated_rows.push_back(m_tabulated_rows.back());
        m_tabulated_rows_dirty = true;
      }
      ImGui::SameLine();
      if (ImGui::Button("Clear Rows")) {
        m_tabulated_rows.clear();
        m_tabulated_strain_grid.clear();
        m_tabulated_rate_grid.clear();
        m_tabulated_temperature_grid.clear();
        m_tabulated_stress_values.clear();
        m_tabulated_rows_dirty = false;
        m_tabulated_csv_error.clear();
      }
      if (ImGui::Button("Apply Rows")) {
        RebuildTabulatedTableFromRows(*this);
      }
      ImGui::Checkbox("Export input as CSV reference", &m_tabulated_export_csv_reference);
      if (m_tabulated_export_csv_reference) {
        ImGui::TextDisabled("The solver input will only contain the CSV path, not the embedded table.");
      } else {
        ImGui::TextDisabled("The solver input will contain the embedded table and will not depend on the CSV file.");
      }
      if (!m_tabulated_csv_error.empty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.45f, 1.0f), "%s", m_tabulated_csv_error.c_str());
      }
      if (m_tabulated_rows_dirty) {
        ImGui::TextDisabled("Rows changed. Click 'Apply Rows' to rebuild the tabulated grid.");
      }
      if (ImGui::BeginTable("##TabulatedRows", 5,
                            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                            ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable,
                            ImVec2(-1.0f, 220.0f))) {
        ImGui::TableSetupColumn("Strain");
        ImGui::TableSetupColumn("Strain Rate");
        ImGui::TableSetupColumn("Temperature");
        ImGui::TableSetupColumn("Stress");
        ImGui::TableSetupColumn("Action");
        ImGui::TableHeadersRow();

        int row_to_remove = -1;
        int row_to_insert_after = -1;
        for (int row_index = 0; row_index < static_cast<int>(m_tabulated_rows.size()); ++row_index) {
          auto &row = m_tabulated_rows[row_index];
          ImGui::PushID(row_index);
          ImGui::TableNextRow();

          ImGui::TableSetColumnIndex(0);
          if (ImGui::InputDouble("##strain", &row.strain, 0.0, 0.0, "%.6g")) {
            m_tabulated_rows_dirty = true;
          }
          ImGui::TableSetColumnIndex(1);
          if (ImGui::InputDouble("##strain_rate", &row.strain_rate, 0.0, 0.0, "%.6g")) {
            m_tabulated_rows_dirty = true;
          }
          ImGui::TableSetColumnIndex(2);
          if (ImGui::InputDouble("##temperature", &row.temperature, 0.0, 0.0, "%.6g")) {
            m_tabulated_rows_dirty = true;
          }
          ImGui::TableSetColumnIndex(3);
          if (ImGui::InputDouble("##stress", &row.stress, 0.0, 0.0, "%.6g")) {
            m_tabulated_rows_dirty = true;
          }
          ImGui::TableSetColumnIndex(4);
          if (ImGui::SmallButton("Add After")) {
            row_to_insert_after = row_index;
          }
          ImGui::SameLine();
          if (ImGui::SmallButton("Remove")) {
            row_to_remove = row_index;
          }
          ImGui::PopID();
        }
        if (row_to_insert_after >= 0) {
          MaterialDialog::TabulatedRow new_row;
          if (!m_tabulated_rows.empty()) {
            new_row = m_tabulated_rows[row_to_insert_after];
          } else {
            new_row.stress = m_yield_stress0;
          }
          m_tabulated_rows.insert(m_tabulated_rows.begin() + row_to_insert_after + 1, new_row);
          m_tabulated_rows_dirty = true;
        }
        if (row_to_remove >= 0) {
          m_tabulated_rows.erase(m_tabulated_rows.begin() + row_to_remove);
          m_tabulated_rows_dirty = true;
        }
        ImGui::EndTable();
      }
      if (!m_tabulated_strain_grid.empty()) {
        ImGui::Text("Grid: %d strain x %d rate x %d temperature",
                    static_cast<int>(m_tabulated_strain_grid.size()),
                    static_cast<int>(m_tabulated_rate_grid.size()),
                    static_cast<int>(m_tabulated_temperature_grid.size()));
        ImGui::InputDouble("Plot Temperature", &m_plot_temperature, 0.0f, 1.0f, "%.4e");
        ImGui::InputDouble("Plot Strain", &m_plot_strain, 0.0f, 1.0f, "%.4f");
        ImGui::Spacing();
        DrawTabulatedPlots(*this);
      } else {
        ImGui::TextDisabled("CSV columns: strain, strain_rate, temperature, stress.");
      }
    }
  }

  if (ImGuiFileDialog::Instance()->Display(kTabulatedMaterialCsvDialogId)) {
    if (ImGuiFileDialog::Instance()->IsOk()) {
      const std::string file_path_name = ImGuiFileDialog::Instance()->GetFilePathName();
      ImportTabulatedCsvIntoDialog(file_path_name, *this);
    }
    ImGuiFileDialog::Instance()->Close();
  }

  if (ImGui::Button("Save to Database")) {
    if (mat != nullptr && mat_db != nullptr && mat_db->isActive()) {
      mat_db->addMaterial(mat);
      mat_db->saveToJson("materials_db.json");
    }
  }

  if (ImGui::Button("Ok")) {
    m_selected_model = item_current;
    if (m_selected_model == TABULATED &&
        m_tabulated_export_csv_reference &&
        std::string(m_tabulated_csv_path.data()).empty()) {
      m_tabulated_csv_error = "CSV export mode requires a valid CSV path.";
      create_material = false;
    } else if (m_selected_model == TABULATED && !RebuildTabulatedTableFromRows(*this)) {
      create_material = false;
    } else {
      create_material = true;
      *p_open = false;
    }

    if (create_material && !mat && !m_temp_mat) {
      m_temp_mat = new Material_();
      mat = m_temp_mat;
    }

    if (create_material && mat) {
      mat->setDensityConstant(m_density_const);
      mat->elastic_m = Elastic_(m_elastic_const, m_poisson_const);
      mat->yieldStress0 = m_yield_stress0;
      mat->strRange = {m_strain_range_min, m_strain_range_max};
      mat->e_min = m_strain_range_min;
      mat->e_max = m_strain_range_max;
      mat->er_min = m_strain_rate_range_min;
      mat->er_max = m_strain_rate_range_max;
      mat->T_min = m_temperature_range_min;
      mat->T_max = m_temperature_range_max;

      if (m_thermal_coupling) {
        mat->cp_T = m_cp_T;
        mat->k_T = m_k_T;
      } else {
        mat->cp_T = 0.0;
        mat->k_T = 0.0;
      }

      if (m_pl != nullptr) {
        delete m_pl;
        m_pl = nullptr;
      }

      if (m_selected_model != NONE) {
        switch (m_selected_model) {
          case BILINEAR:
            m_pl = new Bilinear(bilinear_sy0, bilinear_Et);
            break;

          case HOLLOMON:
            m_pl = new Hollomon(hollomon_K, hollomon_n);
            mat->InitHollomon(mat->Elastic(), m_yield_stress0, hollomon_K, hollomon_n);
            break;

          case JOHNSON_COOK:
            m_pl = new JohnsonCook(jc_B, jc_n, jc_C, jc_eps0, jc_m, jc_Tm, jc_Tt, m_yield_stress0);
            mat->Init_JohnsonCook(mat->Elastic(), m_yield_stress0, jc_B, jc_n, jc_C, jc_eps0, jc_m, jc_Tm, jc_Tt);
            break;

          case _GMT_:
            mat->e_min = ClampPositive(m_strain_range_min);
            mat->e_max = std::max(m_strain_range_max, mat->e_min);
            mat->strRange = {mat->e_min, mat->e_max};
            m_pl = new GMT(gmt_n1, gmt_n2, gmt_C1, gmt_C2, gmt_m1, gmt_m2, gmt_I1, gmt_I2,
                           mat->e_min, mat->e_max,
                           m_strain_rate_range_min, m_strain_rate_range_max,
                           m_temperature_range_min, m_temperature_range_max);
            mat->Init_GMT(mat->Elastic(), gmt_n1, gmt_n2, gmt_C1, gmt_C2, gmt_m1, gmt_m2, gmt_I1, gmt_I2,
                          mat->e_min, mat->e_max,
                          m_strain_rate_range_min, m_strain_rate_range_max,
                          m_temperature_range_min, m_temperature_range_max);
            break;

          case TABULATED:
            m_pl = new Tabulated();
            mat->Material_model = TABULATED;
            mat->tabulated_enabled = true;
            mat->tabulated_export_csv_reference = m_tabulated_export_csv_reference;
            mat->tableCsvPath = m_tabulated_csv_path.data();
            mat->tabulatedStrainGrid = m_tabulated_strain_grid;
            mat->tabulatedRateGrid = m_tabulated_rate_grid;
            mat->tabulatedTemperatureGrid = m_tabulated_temperature_grid;
            mat->tabulatedStressValues = m_tabulated_stress_values;
            if (!mat->tabulatedStrainGrid.empty()) {
              mat->e_min = mat->tabulatedStrainGrid.front();
              mat->e_max = mat->tabulatedStrainGrid.back();
              mat->strRange = {mat->e_min, mat->e_max};
            }
            if (!mat->tabulatedRateGrid.empty()) {
              mat->er_min = mat->tabulatedRateGrid.front();
              mat->er_max = mat->tabulatedRateGrid.back();
            }
            if (!mat->tabulatedTemperatureGrid.empty()) {
              mat->T_min = mat->tabulatedTemperatureGrid.front();
              mat->T_max = mat->tabulatedTemperatureGrid.back();
            }
            break;

          default:
            break;
        }

        delete mat->m_plastic;
        mat->m_plastic = m_pl ? m_pl->clone() : nullptr;
        mat->m_isplastic = (mat->m_plastic != nullptr);
      } else {
        delete mat->m_plastic;
        mat->m_plastic = nullptr;
        mat->m_isplastic = false;
      }
    }
  }

  ImGui::SameLine();
  if (ImGui::Button("Cancel")) {
    cancel_action = true;
    *p_open = false;
  }

  ImGui::End();
}

Material_ ShowCreateMaterialDialog(bool* p_open, MaterialDialog *matdlg, bool *create, Material_Db *mat_db) {
  Material_ ret;
  ImGui::SetNextWindowSize(ImVec2(760, 900), ImGuiCond_FirstUseEver);
  matdlg->Draw("Material", p_open, nullptr, mat_db);

  if (matdlg->isMaterialCreated()) {
    *create = true;
    ret.setDensityConstant(matdlg->m_density_const);

    if (matdlg->m_elastic_const != 0.0 && matdlg->m_poisson_const != 0.0) {
      Elastic_ el(matdlg->m_elastic_const, matdlg->m_poisson_const);
      ret = Material_(el);
      ret.setDensityConstant(matdlg->m_density_const);
      ret.cp_T = matdlg->m_thermal_coupling ? matdlg->m_cp_T : 0.0;
      ret.k_T = matdlg->m_thermal_coupling ? matdlg->m_k_T : 0.0;
      ret.yieldStress0 = matdlg->m_yield_stress0;
      ret.strRange = {matdlg->m_strain_range_min, matdlg->m_strain_range_max};
      ret.e_min = matdlg->m_strain_range_min;
      ret.e_max = matdlg->m_strain_range_max;
      ret.er_min = matdlg->m_strain_rate_range_min;
      ret.er_max = matdlg->m_strain_rate_range_max;
      ret.T_min = matdlg->m_temperature_range_min;
      ret.T_max = matdlg->m_temperature_range_max;
      ret.tableCsvPath = matdlg->m_tabulated_csv_path.data();
      ret.tabulated_export_csv_reference = matdlg->m_tabulated_export_csv_reference;
      ret.tabulatedStrainGrid = matdlg->m_tabulated_strain_grid;
      ret.tabulatedRateGrid = matdlg->m_tabulated_rate_grid;
      ret.tabulatedTemperatureGrid = matdlg->m_tabulated_temperature_grid;
      ret.tabulatedStressValues = matdlg->m_tabulated_stress_values;
      ret.tabulated_enabled = !ret.tabulatedStrainGrid.empty();
      if (matdlg->m_pl != nullptr) {
        ret.m_plastic = matdlg->m_pl->clone();
        ret.m_isplastic = true;
      }
    } else {
      cout << "Material elastic constants should not be zero." << endl;
    }
  }

  return ret;
}

bool ShowEditMaterialDialog(bool* p_open, MaterialDialog *matdlg, Material_ *mat) {
  ImGui::SetNextWindowSize(ImVec2(760, 900), ImGuiCond_FirstUseEver);
  matdlg->Draw("MaterialDlg", p_open, mat);
  return true;
}
