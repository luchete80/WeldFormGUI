#ifndef _LOAD_PLOT_DIALOG_H_
#define _LOAD_PLOT_DIALOG_H_

#include "Dialog.h"

#include <string>
#include <vector>

struct LoadPlotDialog : public Dialog {
  std::string m_csv_path;
  std::string m_error_message;
  std::string m_x_label = "Time";
  std::vector<std::string> m_series_names;
  std::vector<double> m_time_values;
  std::vector<std::vector<double>> m_series_values;

  void SetCsvPath(const std::string& csv_path);
  bool LoadCsv(const std::string& csv_path);
  void Draw(const char* title, bool* p_open = nullptr);
};

#endif
