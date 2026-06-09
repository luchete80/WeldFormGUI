#ifndef _LOAD_PLOT_DIALOG_H_
#define _LOAD_PLOT_DIALOG_H_

#include "Dialog.h"

#include <string>
#include <vector>

struct LoadPlotDialog : public Dialog {
  std::string m_csv_path;
  std::vector<std::string> m_csv_paths;
  std::string m_error_message;
  std::string m_x_label = "Time";
  std::vector<std::string> m_series_names;
  std::vector<bool> m_series_visible;
  std::vector<double> m_time_values;
  std::vector<std::vector<double>> m_series_values;
  bool m_auto_fit_pending = true;

  void SetCsvPath(const std::string& csv_path);
  void SetCsvPaths(const std::vector<std::string>& csv_paths);
  bool LoadCsv(const std::string& csv_path);
  bool LoadCsvFiles(const std::vector<std::string>& csv_paths);
  void Draw(const char* title, bool* p_open = nullptr);
};

#endif
