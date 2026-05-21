#include "bc_dialog.h"
#include <iostream>
#include "Model.h"
#include "BoundaryCondition.h"
#include "InitialCondition.h"
#include "Part.h"
#include "Mesh.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"

#include <algorithm>

namespace {

void ensureAmplitudeTableDefaults(std::vector<double> &timeValues,
                                  std::vector<double> &amplitudeValues) {
    if (timeValues.size() == amplitudeValues.size() && timeValues.size() >= 2)
        return;

    timeValues = {0.0, 1.0};
    amplitudeValues = {1.0, 1.0};
}

void buildAmplitudePlotData(const std::vector<double> &timeValues,
                            const std::vector<double> &amplitudeValues,
                            std::vector<double> &plotTime,
                            std::vector<double> &plotAmplitude) {
    plotTime.clear();
    plotAmplitude.clear();

    const std::size_t pointCount = (std::min)(timeValues.size(), amplitudeValues.size());
    if (pointCount == 0)
        return;

    std::vector<std::pair<double, double>> points;
    points.reserve(pointCount);
    for (std::size_t i = 0; i < pointCount; ++i)
        points.push_back({timeValues[i], amplitudeValues[i]});

    std::stable_sort(points.begin(), points.end(),
                     [](const std::pair<double, double> &lhs, const std::pair<double, double> &rhs) {
                         return lhs.first < rhs.first;
                     });

    plotTime.reserve(pointCount);
    plotAmplitude.reserve(pointCount);
    for (const auto &point : points) {
        plotTime.push_back(point.first);
        plotAmplitude.push_back(point.second);
    }
}

}


//// IS DOUBLE ONLY TO MODIDY PTR.
void BCDialog::Draw(const char* title, bool* p_open, Model* model, Condition **sel_bc,  DialogMode mode) {

    bool isBoundary = false;
    bool isInitial = false;

    if (*sel_bc != nullptr) {
        isBoundary = ((*sel_bc)->kind == ConditionKind::Boundary);
        isInitial  = ((*sel_bc)->kind == ConditionKind::Initial);
    } else {
      if (mode == DialogMode::NewBoundary) {
          isBoundary = true;
      } else if (mode == DialogMode::NewInitial) {
          isInitial = true;
      } else {
          // Auto + sel_bc==nullptr = ERROR
          // Podés decidir default boundary
          isBoundary = true;
      }
    }    

    bool m_isSymmetry = false;
    
    bool create = true;
    //cout << "sel bc "<<*sel_bc<<endl; 
    
    if (!initialized) {
        m_editTarget = *sel_bc;
        m_editingExisting = (m_editTarget != nullptr);
        create = !m_editingExisting;

        if (m_editingExisting) {
          Condition* bc = m_editTarget;  // ✔ obtener BC real
            m_applyTo = (bc->getApplyTo() == ApplyToPart) ? 0 : 1;
            m_targetId = bc->getTargetId();
            m_vel = bc->getValue();
            m_normal = bc->getNormal();
            m_valueType = static_cast<int>(bc->getValueType());
            m_amplitudeFactor = bc->getAmplitudeFactor();
            m_amplitudeTime = bc->getAmplitudeTime();
            m_amplitudeValue = bc->getAmplitudeValue();
            m_dof_mask[0] = bc->getDofMaskX();
            m_dof_mask[1] = bc->getDofMaskY();
            m_dof_mask[2] = bc->getDofMaskZ();
            if (bc->getType() == SymmetryBC) bcType = 2;
            else if (bc->getType() == DisplacementBC) bcType = 1;
            else bcType = 0;

        if (bcType == 2) {
            double3 n = bc->getNormal();
            cout << "Normal"<<n.x<<","<<n.y<<","<<n.z<<endl;
            if (n.x==0 && n.y==0 && n.z==1) symPreset = 0;
            else if (n.x==1 && n.y==0 && n.z==0) symPreset = 1;
            else if (n.x==0 && n.y==1 && n.z==0) symPreset = 2;
            else symPreset = 3;
        }
        
        } else {
          
        m_applyTo = 0;
        m_dof_mask[0] = true;
        m_dof_mask[1] = true;
        m_dof_mask[2] = true;
        m_valueType = 0;
        m_amplitudeFactor = 1.0;
        m_amplitudeTime = {0.0, 1.0};
        m_amplitudeValue = {1.0, 1.0};
        }
        initialized = true;
    }
    else {
        create = !m_editingExisting;
    }

    if (!ImGui::Begin(title, p_open)) {
        ImGui::End();
        return;
    }

    // --- Tipo de BC ---
    ImGui::Text("Boundary Condition Type:");
    ImGui::RadioButton("Velocity", &bcType, 0);
    ImGui::SameLine();
    ImGui::RadioButton("Displacement", &bcType, 1);
    if (isBoundary)
      ImGui::SameLine();
    if (isBoundary)
      ImGui::RadioButton("Symmetry", &bcType, 2);
    
    ImGui::Separator();
            
    // --- Apply To ---
    ImGui::Text("Apply to:");
    ImGui::RadioButton("Part", &m_applyTo, 0);
    ImGui::SameLine();
    ImGui::RadioButton("Set", &m_applyTo, 1);

    // --- Selección de Part ---
    if (m_applyTo == 0) {

        static std::vector<std::string> partNames;
        static int selectedPartIndex = -1;

        int numParts = model->getPartCount();
        if ((int)partNames.size() != numParts) {
            partNames.clear();
            for (int i = 0; i < numParts; i++) {
                std::string name = model->getPart(i)->getName();
                if (name.empty()) name = "Part " + std::to_string(model->getPart(i)->getID());
                partNames.push_back(name);
            }
        }

        if (!create && selectedPartIndex == -1) {
            for (int i = 0; i < numParts; i++) {
                if (model->getPart(i)->getID() == m_targetId) {
                    selectedPartIndex = i;
                    break;
                }
            }
        }

        if (create && selectedPartIndex == -1 && numParts > 0) {
            selectedPartIndex = 0;
            m_targetId = model->getPart(0)->getID();
        }

        const char* preview = 
            (selectedPartIndex >= 0 && selectedPartIndex < numParts)
            ? partNames[selectedPartIndex].c_str()
            : "Select Part";

        if (ImGui::BeginCombo("Select Part##BCDialogPartCombo", preview)) {
            for (int i = 0; i < numParts; i++) {
                bool is_selected = (selectedPartIndex == i);
                if (ImGui::Selectable(partNames[i].c_str(), is_selected)) {
                    selectedPartIndex = i;
                    m_targetId = model->getPart(i)->getID();
                }
                if (is_selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        if (selectedPartIndex >= 0)
            ImGui::Text("Selected ID: %d", model->getPart(selectedPartIndex)->getID());

        // --- SYMMETRY ---
        if (bcType == 2) {
            

            ImGui::Text("Symmetry Plane:");
            ImGui::RadioButton("XY", &symPreset, 0);
            ImGui::SameLine();
            ImGui::RadioButton("YZ", &symPreset, 1);
            ImGui::SameLine();
            ImGui::RadioButton("ZX", &symPreset, 2);
            ImGui::SameLine();
            ImGui::RadioButton("Custom", &symPreset, 3);

            if (symPreset == 0)      m_normal = make_double3(0,0,1);
            else if (symPreset == 1) m_normal = make_double3(1,0,0);
            else if (symPreset == 2) m_normal = make_double3(0,1,0);
            else {
                float n[3] = {
                    static_cast<float>(m_normal.x),
                    static_cast<float>(m_normal.y),
                    static_cast<float>(m_normal.z)
                };
                ImGui::InputFloat3("Normal", n, "%.3f");
                m_normal = make_double3(n[0], n[1], n[2]);
            }
        }
    }
    else {
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
        for (int i = 0; i < static_cast<int>(nodeSets.size()); ++i) {
            if (nodeSets[i].first != nullptr && nodeSets[i].first->getId() == m_targetId) {
                selectedSetIndex = i;
                break;
            }
        }

        if (create && selectedSetIndex == -1 && !nodeSets.empty()) {
            selectedSetIndex = 0;
            m_targetId = nodeSets[0].first->getId();
        }

        const char* preview =
            (selectedSetIndex >= 0 && selectedSetIndex < static_cast<int>(nodeSets.size()))
            ? nodeSets[selectedSetIndex].second.c_str()
            : "Select Node Set";

        if (ImGui::BeginCombo("Select Set##BCDialogSetCombo", preview)) {
            for (int i = 0; i < static_cast<int>(nodeSets.size()); ++i) {
                bool is_selected = (selectedSetIndex == i);
                if (ImGui::Selectable(nodeSets[i].second.c_str(), is_selected)) {
                    selectedSetIndex = i;
                    m_targetId = nodeSets[i].first->getId();
                }
                if (is_selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        if (nodeSets.empty()) {
            ImGui::TextDisabled("No node sets available.");
        } else if (selectedSetIndex >= 0) {
            ImGui::Text("Selected Set ID: %d", m_targetId);
        }
    }

    if (bcType == 0 || bcType == 1) {
        ImGui::Text("Value Mode:");
        ImGui::RadioButton("Constant", &m_valueType, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Function", &m_valueType, 1);

        float v[3] = {
            static_cast<float>(m_vel.x),
            static_cast<float>(m_vel.y),
            static_cast<float>(m_vel.z)
        };
        const char* valueLabel = (bcType == 0)
            ? (m_valueType == 0 ? "Velocity" : "Velocity Base")
            : (m_valueType == 0 ? "Displacement" : "Displacement Base");
        ImGui::InputFloat3(valueLabel, v, "%.3f");
        m_vel = make_double3(v[0], v[1], v[2]);

        if (m_valueType == 1) {
            ensureAmplitudeTableDefaults(m_amplitudeTime, m_amplitudeValue);

            ImGui::SetNextItemWidth(140.0f);
            ImGui::InputDouble("Amplitude Factor", &m_amplitudeFactor, 0.0, 0.0, "%.6g");
            ImGui::TextDisabled("Applied value = base vector * factor * amplitude(t)");

            for (int i = 0; i < static_cast<int>(m_amplitudeTime.size()); ++i) {
                ImGui::PushID(i);
                ImGui::SetNextItemWidth(120.0f);
                ImGui::InputDouble("Time", &m_amplitudeTime[i], 0.0, 0.0, "%.6g");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(120.0f);
                ImGui::InputDouble("Scale", &m_amplitudeValue[i], 0.0, 0.0, "%.6g");
                ImGui::PopID();
            }

            if (ImGui::Button("Add Point")) {
                const double nextTime = m_amplitudeTime.empty() ? 0.0 : (m_amplitudeTime.back() + 1.0);
                const double nextValue = m_amplitudeValue.empty() ? 1.0 : m_amplitudeValue.back();
                m_amplitudeTime.push_back(nextTime);
                m_amplitudeValue.push_back(nextValue);
            }
            ImGui::SameLine();
            if (ImGui::Button("Remove Point") && m_amplitudeTime.size() > 2) {
                m_amplitudeTime.pop_back();
                m_amplitudeValue.pop_back();
            }

            std::vector<double> plotTime;
            std::vector<double> plotAmplitude;
            buildAmplitudePlotData(m_amplitudeTime, m_amplitudeValue, plotTime, plotAmplitude);
            if (!plotTime.empty() &&
                ImPlot::BeginPlot("##BCAmplitudePlot", ImVec2(-1.0f, 220.0f))) {
                ImPlot::SetupAxes("Time", "Scale");
                if (plotTime.size() >= 2) {
                    ImPlot::PlotLine("Amplitude", plotTime.data(), plotAmplitude.data(),
                                     static_cast<int>(plotTime.size()));
                }
                ImPlot::PlotScatter("Points", plotTime.data(), plotAmplitude.data(),
                                    static_cast<int>(plotTime.size()));
                ImPlot::EndPlot();
            }
        }

        ImGui::Text("Apply DOFs:");
        ImGui::Checkbox("X", &m_dof_mask[0]);
        ImGui::SameLine();
        ImGui::Checkbox("Y", &m_dof_mask[1]);
        ImGui::SameLine();
        ImGui::Checkbox("Z", &m_dof_mask[2]);
    } else {
        ImGui::TextDisabled("Value and DOF mask are ignored for Symmetry BC.");
    }

    // --- BOTÓN OK / CREATE ---
    std::string butleg = create ? "Create" : "Update";
    if (ImGui::Button(butleg.c_str())) {


        BCApplyTo target = (m_applyTo == 0) ? ApplyToPart : ApplyToNodeSet;
        BoundaryCondition *bc = nullptr;
        InitialCondition  *ic = nullptr;

        if (create) {
          if (isBoundary){
            if (bcType == 0)
                bc = new BoundaryCondition(VelocityBC, target, m_targetId, m_vel);
            else if (bcType == 1)
                bc = new BoundaryCondition(DisplacementBC, target, m_targetId, m_vel);
            else
                bc = new BoundaryCondition(SymmetryBC, target, m_targetId, m_normal);

            if (bc != nullptr && bcType != 2) {
              bc->setValueType(m_valueType);
              bc->setAmplitudeFactor(m_amplitudeFactor);
              bc->setAmplitudeTable(m_amplitudeTime, m_amplitudeValue);
              bc->setDofMask(m_dof_mask[0], m_dof_mask[1], m_dof_mask[2]);
            }

            model->addBoundaryCondition(bc);
          } else if (isInitial){
            ic = new InitialCondition(VelocityBC, target, m_targetId, m_vel);
            if (ic != nullptr) {
              ic->setValueType(m_valueType);
              ic->setAmplitudeFactor(m_amplitudeFactor);
              ic->setAmplitudeTable(m_amplitudeTime, m_amplitudeValue);
              ic->setDofMask(m_dof_mask[0], m_dof_mask[1], m_dof_mask[2]);
            }
          
          }
          if (ic != nullptr) {
            model->addInitialCondition(ic);
          }
        }
        else {
            Condition* bc = m_editTarget;      // ✔ puntero real
            if (bc == nullptr) {
                *p_open = false;
                ImGui::End();
                initialized = false;
                m_editingExisting = false;
                m_editTarget = nullptr;
                return;
            }
            
            bc->setApplyTo(target);
            bc->setTargetId(m_targetId);

            if (bcType == 0) {
                bc->setType(VelocityBC);
                bc->setValue(m_vel);
                bc->setValueType(m_valueType);
                bc->setAmplitudeFactor(m_amplitudeFactor);
                bc->setAmplitudeTable(m_amplitudeTime, m_amplitudeValue);
                bc->setDofMask(m_dof_mask[0], m_dof_mask[1], m_dof_mask[2]);
            } else if (bcType == 1) {
                bc->setType(DisplacementBC);
                bc->setValue(m_vel);
                bc->setValueType(m_valueType);
                bc->setAmplitudeFactor(m_amplitudeFactor);
                bc->setAmplitudeTable(m_amplitudeTime, m_amplitudeValue);
                bc->setDofMask(m_dof_mask[0], m_dof_mask[1], m_dof_mask[2]);
            } else {
                bc->setType(SymmetryBC);
                bc->setNormal(m_normal);
            }
        }

        std::cout << (create ? "Created" : "Updated") << " BC\n";
        *p_open = false;
        *sel_bc = nullptr;
    }

    // Cancel
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
        *p_open = false;
        //sel_bc = nullptr;
    }

    if (!(*p_open)) {
        initialized = false;
        m_editingExisting = false;
        m_editTarget = nullptr;
        *sel_bc = nullptr;
        cout << "closed. sel_çbc "<<*sel_bc<<endl;
    }

    ImGui::End();
}
