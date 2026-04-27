#pragma once

#include <string>
#include <vector>

struct DemoEntry {
    std::string name;
    std::string path;
    std::string description;
};

class DemoDialog {
public:
    void SetDemoRoot(const std::string& demoRoot);
    void Draw(const char* title, bool* p_open);

    std::string selectedDemoPath;

    std::string ConsumeSelectedDemoPath();
    
private:
    std::string m_demoRoot;
    std::vector<DemoEntry> m_demos;
    std::string m_error;

    void Refresh();
    std::string ExtractDescription(const std::string& readmePath);
    std::string Trim(const std::string& value);
};