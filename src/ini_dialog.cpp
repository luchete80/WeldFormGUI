#include "ini_dialog.h"

#include <string>
#include <utility>
#include <vector>

#include "imgui.h"

#include "model/InitialCondition.h"
#include "model/Model.h"
#include "model/Mesh.h"
#include "model/Part.h"

void IniDialog::Draw(const char* title, bool* p_open, Model* model, InitialCondition** sel_ic)
{
    const bool create = (sel_ic == nullptr || *sel_ic == nullptr);

    if (!initialized) {
        if (!create && *sel_ic != nullptr) {
            InitialCondition* ic = *sel_ic;
            m_applyTo = (ic->getApplyTo() == ApplyToPart) ? 0 : 1;
            m_targetId = ic->getTargetId();
            m_velocity = ic->getValue();
            m_temperature = ic->getValueX();
            m_dof_mask[0] = ic->getDofMaskX();
            m_dof_mask[1] = ic->getDofMaskY();
            m_dof_mask[2] = ic->getDofMaskZ();
            m_iniType = (ic->getType() == TempIC || ic->getType() == TempBC) ? 1 : 0;
        } else {
            m_applyTo = 0;
            m_targetId = -1;
            m_iniType = 0;
            m_velocity = make_double3(0, 0, 0);
            m_temperature = 20.0;
            m_dof_mask[0] = true;
            m_dof_mask[1] = true;
            m_dof_mask[2] = true;
        }
        initialized = true;
    }

    if (!ImGui::Begin(title, p_open)) {
        ImGui::End();
        return;
    }

    ImGui::Text("Initial Condition Type:");
    ImGui::RadioButton("Velocity", &m_iniType, 0);
    ImGui::SameLine();
    ImGui::RadioButton("Temperature", &m_iniType, 1);

    ImGui::Separator();
    ImGui::Text("Apply to:");
    ImGui::RadioButton("Part", &m_applyTo, 0);
    ImGui::SameLine();
    ImGui::RadioButton("Set", &m_applyTo, 1);

    if (m_applyTo == 0) {
        std::vector<std::string> partNames;
        int selectedPartIndex = -1;

        for (int i = 0; i < model->getPartCount(); ++i) {
            Part* part = model->getPart(i);
            if (part == nullptr) {
                continue;
            }
            std::string name = part->getName();
            if (name.empty()) {
                name = "Part " + std::to_string(part->getID());
            }
            partNames.push_back(name);
            if (part->getID() == m_targetId) {
                selectedPartIndex = i;
            }
        }

        if (selectedPartIndex == -1 && model->getPartCount() > 0) {
            selectedPartIndex = 0;
            m_targetId = model->getPart(0)->getID();
        }

        const char* preview =
            (selectedPartIndex >= 0 && selectedPartIndex < (int)partNames.size())
            ? partNames[selectedPartIndex].c_str()
            : "Select Part";

        if (ImGui::BeginCombo("Select Part##IniDialogPartCombo", preview)) {
            for (int i = 0; i < model->getPartCount(); ++i) {
                Part* part = model->getPart(i);
                if (part == nullptr) {
                    continue;
                }
                const bool isSelected = (part->getID() == m_targetId);
                const char* rawName = part->getName();
                std::string label = (rawName == nullptr || rawName[0] == '\0')
                  ? ("Part " + std::to_string(part->getID()))
                  : std::string(rawName);
                if (ImGui::Selectable(label.c_str(), isSelected)) {
                    m_targetId = part->getID();
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    } else {
        std::vector<std::pair<NodeSet*, std::string>> nodeSets;
        for (int i = 0; i < model->getPartCount(); ++i) {
            Part* part = model->getPart(i);
            if (part == nullptr || part->getMesh() == nullptr) {
                continue;
            }

            Mesh* mesh = part->getMesh();
            for (int s = 0; s < mesh->getNodeSetCount(); ++s) {
                NodeSet& nodeSet = mesh->getNodeSet(s);
                std::string label = nodeSet.getLabel().empty()
                  ? ("Node Set " + std::to_string(nodeSet.getId()))
                  : nodeSet.getLabel();
                label += " [id " + std::to_string(nodeSet.getId()) + "]";
                nodeSets.push_back({&nodeSet, label});
            }
        }

        int selectedSetIndex = -1;
        for (int i = 0; i < (int)nodeSets.size(); ++i) {
            if (nodeSets[i].first != nullptr && nodeSets[i].first->getId() == m_targetId) {
                selectedSetIndex = i;
                break;
            }
        }

        if (selectedSetIndex == -1 && !nodeSets.empty()) {
            selectedSetIndex = 0;
            m_targetId = nodeSets[0].first->getId();
        }

        const char* preview =
            (selectedSetIndex >= 0 && selectedSetIndex < (int)nodeSets.size())
            ? nodeSets[selectedSetIndex].second.c_str()
            : "Select Node Set";

        if (ImGui::BeginCombo("Select Set##IniDialogSetCombo", preview)) {
            for (int i = 0; i < (int)nodeSets.size(); ++i) {
                const bool isSelected = (selectedSetIndex == i);
                if (ImGui::Selectable(nodeSets[i].second.c_str(), isSelected)) {
                    m_targetId = nodeSets[i].first->getId();
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        if (nodeSets.empty()) {
            ImGui::TextDisabled("No node sets available.");
        }
    }

    ImGui::Separator();
    if (m_iniType == 0) {
        float v[3] = {
            static_cast<float>(m_velocity.x),
            static_cast<float>(m_velocity.y),
            static_cast<float>(m_velocity.z)
        };
        ImGui::InputFloat3("Velocity", v, "%.3f");
        m_velocity = make_double3(v[0], v[1], v[2]);

        ImGui::Text("Apply DOFs:");
        ImGui::Checkbox("X", &m_dof_mask[0]);
        ImGui::SameLine();
        ImGui::Checkbox("Y", &m_dof_mask[1]);
        ImGui::SameLine();
        ImGui::Checkbox("Z", &m_dof_mask[2]);
    } else {
        float temp = static_cast<float>(m_temperature);
        ImGui::InputFloat("Temperature", &temp, 0.0f, 0.0f, "%.3f");
        m_temperature = temp;
        ImGui::TextDisabled("Temperature is stored as a scalar initial condition.");
    }

    const char* buttonLabel = create ? "Create" : "OK";
    if (ImGui::Button(buttonLabel)) {
        const BCApplyTo target = (m_applyTo == 0) ? ApplyToPart : ApplyToNodeSet;
        const BCType type = (m_iniType == 0) ? VelocityIC : TempIC;
        const double3 value = (m_iniType == 0)
            ? m_velocity
            : make_double3(m_temperature, 0.0, 0.0);

        if (create) {
            InitialCondition* ic = new InitialCondition(type, target, m_targetId, value);
            if (m_iniType == 0) {
                ic->setDofMask(m_dof_mask[0], m_dof_mask[1], m_dof_mask[2]);
            }
            model->addInitialCondition(ic);
        } else if (sel_ic != nullptr && *sel_ic != nullptr) {
            InitialCondition* ic = *sel_ic;
            ic->setType(type);
            ic->setApplyTo(target);
            ic->setTargetId(m_targetId);
            ic->setValue(value);
            if (m_iniType == 0) {
                ic->setDofMask(m_dof_mask[0], m_dof_mask[1], m_dof_mask[2]);
            } else {
                ic->setDofMask(true, false, false);
            }
        }

        *p_open = false;
        if (sel_ic != nullptr) {
            *sel_ic = nullptr;
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
        *p_open = false;
    }

    if (!(*p_open)) {
        initialized = false;
        if (sel_ic != nullptr) {
            *sel_ic = nullptr;
        }
    }

    ImGui::End();
}
