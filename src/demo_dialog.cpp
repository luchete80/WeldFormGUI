#include "demo_dialog.h"

#include "imgui.h"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

//-----------------------------------

void DemoDialog::SetDemoRoot(const std::string& demoRoot) {
    if (demoRoot == m_demoRoot && !m_demos.empty())
        return;

    m_demoRoot = demoRoot;
    Refresh();
}

//-----------------------------------

void DemoDialog::Refresh() {
    m_demos.clear();
    m_error.clear();

    if (m_demoRoot.empty()) {
        m_error = "Demo root path is empty.";
        return;
    }

    if (!fs::exists(m_demoRoot)) {
        m_error = "Demo folder not found: " + m_demoRoot;
        return;
    }

    for (const auto& entry : fs::directory_iterator(m_demoRoot)) {

        if (!entry.is_directory())
            continue;

        DemoEntry demo;

        demo.path = entry.path().string();
        demo.name = entry.path().filename().string();

        fs::path readme = entry.path() / "README.md";
        demo.description = ExtractDescription(readme.string());

        m_demos.push_back(demo);
    }
}

//-----------------------------------

std::string DemoDialog::ExtractDescription(const std::string& readmePath) {

    std::ifstream file(readmePath);
    if (!file.is_open())
        return "";

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string text = buffer.str();

    const std::string beginTag = "[DESC]";
    const std::string endTag   = "[/DESC]";

    size_t begin = text.find(beginTag);
    size_t end   = text.find(endTag);

    if (begin == std::string::npos || end == std::string::npos || end <= begin)
        return "";

    begin += beginTag.size();

    return Trim(text.substr(begin, end - begin));
}

//-----------------------------------

std::string DemoDialog::Trim(const std::string& value) {

    const std::string whitespace = " \t\r\n";

    size_t begin = value.find_first_not_of(whitespace);
    if (begin == std::string::npos)
        return "";

    size_t end = value.find_last_not_of(whitespace);

    return value.substr(begin, end - begin + 1);
}

//-----------------------------------

void DemoDialog::Draw(const char* title, bool* p_open) {

    if (p_open && !*p_open)
        return;

    ImGui::SetNextWindowSize(ImVec2(650, 420), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin(title, p_open)) {
        ImGui::End();
        return;
    }

    ImGui::TextWrapped("Select a demo case:");
    ImGui::Spacing();

    if (ImGui::Button("Refresh")) {
        Refresh();
    }

    if (!m_error.empty()) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "%s", m_error.c_str());
    }

    ImGui::Separator();

    if (m_demos.empty()) {
        ImGui::TextDisabled("No demos found.");
        ImGui::End();
        return;
    }

    for (const auto& demo : m_demos) {

        ImGui::PushID(demo.path.c_str());

        ImGui::BeginChild(demo.name.c_str(), ImVec2(0, 110), true);

        ImGui::Text("%s", demo.name.c_str());
        ImGui::Spacing();

        if (!demo.description.empty())
            ImGui::TextWrapped("%s", demo.description.c_str());
        else
            ImGui::TextDisabled("No description available.");

        ImGui::Spacing();

        if (ImGui::Button("Load Demo")) {
            selectedDemoPath = demo.path;
        }

        ImGui::EndChild();

        ImGui::PopID();
        ImGui::Spacing();
    }

    ImGui::End();
}

std::string DemoDialog::ConsumeSelectedDemoPath() {
    std::string path = selectedDemoPath;
    selectedDemoPath.clear();
    return path;
}