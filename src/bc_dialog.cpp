#include "bc_dialog.h"
#include <iostream>
#include "Model.h"
#include "BoundaryCondition.h"
#include "InitialCondition.h"
#include "Part.h"
#include "Step.h"
#include "Mesh.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"

#include <algorithm>
#include <cmath>

namespace {

bool finiteVector(const double3 &value);

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

bool angularTableIsValid(const std::vector<double> &times,
                         const std::vector<double> &values,
                         std::string &error) {
    if (times.size() != values.size() || times.size() < 2) {
        error = "La tabla angular requiere al menos dos puntos y longitudes iguales.";
        return false;
    }
    for (std::size_t i = 0; i < times.size(); ++i) {
        if (!std::isfinite(times[i]) || times[i] < 0.0) {
            error = "Los tiempos angulares deben ser finitos y no negativos.";
            return false;
        }
        if (!std::isfinite(values[i])) {
            error = "Los valores de la tabla angular deben ser finitos.";
            return false;
        }
        if (i > 0 && !(times[i] > times[i - 1])) {
            error = "Los tiempos angulares deben crecer estrictamente.";
            return false;
        }
    }
    return true;
}

bool validateAngularMotionWithModelContract(const double3 &velocity,
                                            const double3 &rotationCenter,
                                            int valueType,
                                            double amplitudeFactor,
                                            const std::vector<double> &times,
                                            const std::vector<double> &values,
                                            int targetId,
                                            std::string &error) {
    BoundaryCondition candidate(VelocityBC, ApplyToPart, targetId, velocity);
    candidate.setAngularVelocity(velocity);
    candidate.setRotationCenter(rotationCenter);
    candidate.setAngularValueType(valueType);
    candidate.setAngularAmplitudeFactor(amplitudeFactor);
    candidate.setAngularAmplitudeTable(valueType == 1 ? times : std::vector<double>(),
                                       valueType == 1 ? values : std::vector<double>());
    return candidate.isAngularMotionValid(&error);
}

bool supportsAngularMotion(Model *model) {
    if (model == nullptr || model->getAnalysisType() != Solid3D ||
        model->getDimension() != 3 || model->getStepCount() <= 0)
        return false;
    Step *step = model->getStep(0);
    return step != nullptr && step->isImplicit() &&
        step->m_implicitFormulation == ImplicitFormulation::RigidViscoplastic;
}

bool finiteLinearValues(const double3 &velocity,
                        int valueType,
                        double amplitudeFactor,
                        const std::vector<double> &times,
                        const std::vector<double> &values) {
    if (!finiteVector(velocity) || !std::isfinite(amplitudeFactor))
        return false;
    if (valueType == 1) {
        for (double value : times)
            if (!std::isfinite(value)) return false;
        for (double value : values)
            if (!std::isfinite(value)) return false;
    }
    return true;
}

bool finiteVector(const double3 &value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

}

void BCDialog::resetDialogState() {
    initialized = false;
    m_editingExisting = false;
    m_editTarget = nullptr;
    m_targetId = -1;
    m_applyTo = 0;
    m_vel = make_double3(0.0, 0.0, 0.0);
    m_dof_mask[0] = m_dof_mask[1] = m_dof_mask[2] = true;
    m_valueType = 0;
    m_amplitudeFactor = 1.0;
    m_amplitudeTime = {0.0, 1.0};
    m_amplitudeValue = {1.0, 1.0};
    m_activeAmplitudeRow = -1;
    m_angularMotionEnabled = false;
    m_angularVelocity = make_double3(0.0, 0.0, 0.0);
    m_rotationCenter = make_double3(0.0, 0.0, 0.0);
    m_angularValueType = 0;
    m_angularAmplitudeFactor = 1.0;
    m_angularAmplitudeTime = {0.0, 1.0};
    m_angularAmplitudeValue = {1.0, 1.0};
    m_activeAngularAmplitudeRow = -1;
    m_angularValidationError.clear();
    m_angularSaveAttempted = false;
    bcType = 0;
    m_normal = make_double3(0.0, 0.0, 1.0);
    symPreset = 0;
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
            if (bc->kind == ConditionKind::Boundary) {
                BoundaryCondition *boundary = static_cast<BoundaryCondition *>(bc);
                m_angularMotionEnabled = boundary->hasAngularMotion();
                m_angularVelocity = boundary->getAngularVelocity();
                m_rotationCenter = boundary->getRotationCenter();
                m_angularValueType = static_cast<int>(boundary->getAngularValueType());
                m_angularAmplitudeFactor = boundary->getAngularAmplitudeFactor();
                m_angularAmplitudeTime = boundary->getAngularAmplitudeTime();
                m_angularAmplitudeValue = boundary->getAngularAmplitudeValue();
            }
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
        m_angularMotionEnabled = false;
        m_angularVelocity = make_double3(0.0, 0.0, 0.0);
        m_rotationCenter = make_double3(0.0, 0.0, 0.0);
        m_angularValueType = 0;
        m_angularAmplitudeFactor = 1.0;
        m_angularAmplitudeTime = {0.0, 1.0};
        m_angularAmplitudeValue = {1.0, 1.0};
        m_angularSaveAttempted = false;
        m_angularValidationError.clear();
        m_activeAmplitudeRow = -1;
        m_activeAngularAmplitudeRow = -1;
        m_vel = make_double3(0.0, 0.0, 0.0);
        m_normal = make_double3(0.0, 0.0, 1.0);
        m_targetId = -1;
        if (model != nullptr && model->getPartCount() > 0)
            m_targetId = model->getPart(0)->getID();
        }
        initialized = true;
    }
    else {
        create = !m_editingExisting;
    }

    bool modelHasAngularMotion = false;
    if (model != nullptr) {
        for (int i = 0; i < model->getBCCount(); ++i) {
            BoundaryCondition *boundary = model->getBC(i);
            if (boundary != nullptr && boundary->hasAngularMotion()) {
                modelHasAngularMotion = true;
                break;
            }
        }
    }

    if (!ImGui::Begin(title, p_open)) {
        ImGui::End();
        if (!*p_open) {
            resetDialogState();
            *sel_bc = nullptr;
        }
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
        std::vector<std::string> partNames;
        int numParts = model->getPartCount();
        int selectedPartIndex = -1;
        for (int i = 0; i < numParts; i++) {
            Part *part = model->getPart(i);
            std::string name = part->getName();
            if (name.empty()) name = "Part " + std::to_string(part->getID());
            partNames.push_back(name + " [id " + std::to_string(part->getID()) + "]");
            if (part->getID() == m_targetId)
                selectedPartIndex = i;
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
            if (!m_angularMotionEnabled && !modelHasAngularMotion)
                ensureAmplitudeTableDefaults(m_amplitudeTime, m_amplitudeValue);

            ImGui::SetNextItemWidth(140.0f);
            ImGui::InputDouble("Amplitude Factor", &m_amplitudeFactor, 0.0, 0.0, "%.6g");
            ImGui::TextDisabled("Applied value = base vector * factor * amplitude(t)");

            const std::size_t lineRowCount = (std::min)(m_amplitudeTime.size(), m_amplitudeValue.size());
            for (int i = 0; i < static_cast<int>(lineRowCount); ++i) {
                ImGui::PushID(i);
                ImGui::SetNextItemWidth(120.0f);
                ImGui::InputDouble("Time", &m_amplitudeTime[i], 0.0, 0.0, "%.6g");
                if (ImGui::IsItemActive() || ImGui::IsItemFocused()) {
                    m_activeAmplitudeRow = i;
                }
                ImGui::SameLine();
                ImGui::SetNextItemWidth(120.0f);
                ImGui::InputDouble("Scale", &m_amplitudeValue[i], 0.0, 0.0, "%.6g");
                if (ImGui::IsItemActive() || ImGui::IsItemFocused()) {
                    m_activeAmplitudeRow = i;
                }
                ImGui::PopID();
            }
            if (m_amplitudeTime.size() != m_amplitudeValue.size()) {
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.2f, 1.0f),
                                   "La tabla lineal tiene longitudes distintas.");
                if (ImGui::Button("Reset Linear Table")) {
                    m_amplitudeTime = {0.0, 1.0};
                    m_amplitudeValue = {1.0, 1.0};
                }
            }

            const bool lineTableLengthsMatch = m_amplitudeTime.size() == m_amplitudeValue.size();
            ImGui::BeginDisabled(!lineTableLengthsMatch);
            if (ImGui::Button("Add Point Below")) {
                const int insertIndex =
                    (m_activeAmplitudeRow >= 0 &&
                     m_activeAmplitudeRow < static_cast<int>(m_amplitudeTime.size()))
                    ? (m_activeAmplitudeRow + 1)
                    : static_cast<int>(m_amplitudeTime.size());

                double nextTime = 0.0;
                double nextValue = 1.0;
                if (m_amplitudeTime.empty()) {
                    nextTime = 0.0;
                    nextValue = 1.0;
                } else if (insertIndex <= 0) {
                    nextTime = m_amplitudeTime.front();
                    nextValue = m_amplitudeValue.front();
                } else if (insertIndex >= static_cast<int>(m_amplitudeTime.size())) {
                    nextTime = m_amplitudeTime.back() + 1.0;
                    nextValue = m_amplitudeValue.back();
                } else {
                    const double previousTime = m_amplitudeTime[insertIndex - 1];
                    const double nextExistingTime = m_amplitudeTime[insertIndex];
                    nextTime = previousTime + (nextExistingTime - previousTime) * 0.5;
                    nextValue = m_amplitudeValue[insertIndex - 1];
                }

                m_amplitudeTime.insert(m_amplitudeTime.begin() + insertIndex, nextTime);
                m_amplitudeValue.insert(m_amplitudeValue.begin() + insertIndex, nextValue);
                m_activeAmplitudeRow = insertIndex;
            }
            ImGui::SameLine();
            if (ImGui::Button("Remove Point") && m_amplitudeTime.size() > 2) {
                const int removeIndex =
                    (m_activeAmplitudeRow >= 0 &&
                     m_activeAmplitudeRow < static_cast<int>(m_amplitudeTime.size()))
                    ? m_activeAmplitudeRow
                    : (static_cast<int>(m_amplitudeTime.size()) - 1);
                m_amplitudeTime.erase(m_amplitudeTime.begin() + removeIndex);
                m_amplitudeValue.erase(m_amplitudeValue.begin() + removeIndex);
                m_activeAmplitudeRow = std::min(removeIndex,
                                                static_cast<int>(m_amplitudeTime.size()) - 1);
            }
            ImGui::EndDisabled();

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

    Part *angularTargetPart = nullptr;
    if (m_applyTo == 0) {
        for (int i = 0; i < model->getPartCount(); ++i) {
            Part *part = model->getPart(i);
            if (part != nullptr && part->getID() == m_targetId) {
                angularTargetPart = part;
                break;
            }
        }
    }
    const bool angularDestinationCompatible =
        isBoundary && bcType == 0 && m_applyTo == 0 && supportsAngularMotion(model) &&
        angularTargetPart != nullptr && angularTargetPart->getType() == Rigid;
    const bool hasExistingAngularMotion =
        m_editTarget != nullptr && m_editTarget->kind == ConditionKind::Boundary &&
        static_cast<BoundaryCondition *>(m_editTarget)->hasAngularMotion();

    if (angularDestinationCompatible || hasExistingAngularMotion || m_angularMotionEnabled) {
        ImGui::Separator();
        ImGui::Text("Angular velocity (rad/s; right-hand rule, global X/Y/Z)");
        ImGui::Checkbox("Enable angular velocity##BCAngular", &m_angularMotionEnabled);
        if (angularDestinationCompatible) {
            const bool angularEditorEnabled = m_angularMotionEnabled;
            ImGui::BeginDisabled(!angularEditorEnabled);
            ImGui::Text("Angular value mode:");
            ImGui::RadioButton("Constant##BCAngular", &m_angularValueType, 0);
            ImGui::SameLine();
            ImGui::RadioButton("Function##BCAngular", &m_angularValueType, 1);

            double *angularComponents[3] = {&m_angularVelocity.x, &m_angularVelocity.y, &m_angularVelocity.z};
            double *centerComponents[3] = {&m_rotationCenter.x, &m_rotationCenter.y, &m_rotationCenter.z};
            const char *axisNames[3] = {"X", "Y", "Z"};
            for (int axis = 0; axis < 3; ++axis) {
                ImGui::SetNextItemWidth(150.0f);
                ImGui::InputDouble((std::string("Angular ") + axisNames[axis] + " (rad/s)##BCAngular").c_str(),
                                   angularComponents[axis], 0.0, 0.0, "%.17g");
            }
            ImGui::Text("Rotation center (global coordinates)");
            for (int axis = 0; axis < 3; ++axis) {
                ImGui::SetNextItemWidth(150.0f);
                ImGui::InputDouble((std::string("Center ") + axisNames[axis] + "##BCAngular").c_str(),
                                   centerComponents[axis], 0.0, 0.0, "%.17g");
            }
            ImGui::TextDisabled("Centro inicial explícito; se traslada con la pieza.");
            ImGui::TextDisabled("La tabla escala las tres componentes angulares.");

            if (m_angularValueType == 1) {
                ImGui::SetNextItemWidth(140.0f);
                ImGui::InputDouble("Angular amplitude factor##BCAngular", &m_angularAmplitudeFactor,
                                   0.0, 0.0, "%.6g");
                ImGui::TextDisabled("Interpolación lineal; se mantiene el valor extremo fuera del intervalo.");
                const std::size_t angularRowCount =
                    (std::min)(m_angularAmplitudeTime.size(), m_angularAmplitudeValue.size());
                for (int i = 0; i < static_cast<int>(angularRowCount); ++i) {
                    ImGui::PushID("BCAngularRow");
                    ImGui::PushID(i);
                    ImGui::SetNextItemWidth(120.0f);
                    ImGui::InputDouble("Time##BCAngular", &m_angularAmplitudeTime[i], 0.0, 0.0, "%.6g");
                    if (ImGui::IsItemActive() || ImGui::IsItemFocused()) m_activeAngularAmplitudeRow = i;
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(120.0f);
                    ImGui::InputDouble("Scale##BCAngular", &m_angularAmplitudeValue[i], 0.0, 0.0, "%.6g");
                    if (ImGui::IsItemActive() || ImGui::IsItemFocused()) m_activeAngularAmplitudeRow = i;
                    ImGui::PopID();
                    ImGui::PopID();
                }
                if (m_angularAmplitudeTime.size() != m_angularAmplitudeValue.size() ||
                    m_angularAmplitudeTime.size() < 2) {
                    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.2f, 1.0f),
                        "La tabla angular es inválida; debe tener al menos dos filas y longitudes iguales.");
                    if (ImGui::Button("Reset Angular Table")) {
                        m_angularAmplitudeTime = {0.0, 1.0};
                        m_angularAmplitudeValue = {1.0, 1.0};
                        m_activeAngularAmplitudeRow = -1;
                    }
                }
                const bool angularTableLengthsMatch =
                    m_angularAmplitudeTime.size() == m_angularAmplitudeValue.size();
                ImGui::BeginDisabled(!angularTableLengthsMatch);
                if (ImGui::Button("Add Angular Point Below")) {
                    const int insertIndex = (m_activeAngularAmplitudeRow >= 0 &&
                        m_activeAngularAmplitudeRow < static_cast<int>(m_angularAmplitudeTime.size()))
                        ? m_activeAngularAmplitudeRow + 1 : static_cast<int>(m_angularAmplitudeTime.size());
                    double nextTime = m_angularAmplitudeTime.empty() ? 0.0 : m_angularAmplitudeTime.back() + 1.0;
                    double nextValue = m_angularAmplitudeValue.empty() ? 1.0 : m_angularAmplitudeValue.back();
                    if (insertIndex > 0 && insertIndex < static_cast<int>(m_angularAmplitudeTime.size())) {
                        const double previous = m_angularAmplitudeTime[insertIndex - 1];
                        nextTime = previous + (m_angularAmplitudeTime[insertIndex] - previous) * 0.5;
                        nextValue = m_angularAmplitudeValue[insertIndex - 1];
                    }
                    m_angularAmplitudeTime.insert(m_angularAmplitudeTime.begin() + insertIndex, nextTime);
                    m_angularAmplitudeValue.insert(m_angularAmplitudeValue.begin() + insertIndex, nextValue);
                    m_activeAngularAmplitudeRow = insertIndex;
                }
                ImGui::SameLine();
                if (ImGui::Button("Remove Angular Point") && m_angularAmplitudeTime.size() > 2) {
                    const int removeIndex = (m_activeAngularAmplitudeRow >= 0 &&
                        m_activeAngularAmplitudeRow < static_cast<int>(m_angularAmplitudeTime.size()))
                        ? m_activeAngularAmplitudeRow : static_cast<int>(m_angularAmplitudeTime.size()) - 1;
                    m_angularAmplitudeTime.erase(m_angularAmplitudeTime.begin() + removeIndex);
                    m_angularAmplitudeValue.erase(m_angularAmplitudeValue.begin() + removeIndex);
                    m_activeAngularAmplitudeRow = std::min(removeIndex,
                        static_cast<int>(m_angularAmplitudeTime.size()) - 1);
                }
                ImGui::EndDisabled();
                std::vector<double> plotTime, plotAmplitude;
                std::string plotError;
                const bool plotTableValid = angularTableIsValid(
                    m_angularAmplitudeTime, m_angularAmplitudeValue, plotError);
                if (plotTableValid)
                    buildAmplitudePlotData(m_angularAmplitudeTime, m_angularAmplitudeValue, plotTime, plotAmplitude);
                if (plotTableValid && plotTime.size() >= 2 && ImPlot::BeginPlot("##BCAngularAmplitudePlot", ImVec2(-1.0f, 180.0f))) {
                    ImPlot::SetupAxes("Time", "Scale");
                    ImPlot::PlotLine("Angular amplitude", plotTime.data(), plotAmplitude.data(),
                                     static_cast<int>(plotTime.size()));
                    ImPlot::PlotScatter("Points", plotTime.data(), plotAmplitude.data(),
                                         static_cast<int>(plotTime.size()));
                    ImPlot::EndPlot();
                }
            }
            ImGui::EndDisabled();
        } else if (m_angularMotionEnabled || hasExistingAngularMotion) {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.2f, 1.0f),
                "El giro requiere Velocity en una Part rígida y un modelo 3D MINI3D implícito. Cambie el destino o desactive el giro.");
        }
    }
    if (m_angularSaveAttempted && !m_angularValidationError.empty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.25f, 0.25f, 1.0f), "%s", m_angularValidationError.c_str());
    }

    // --- BOTÓN OK / CREATE ---
    std::string butleg = create ? "Create" : "Update";
    if (ImGui::Button(butleg.c_str())) {
        m_angularSaveAttempted = true;

        m_angularValidationError.clear();
        const bool angularEnabledOnSave = m_angularMotionEnabled;
        if (angularEnabledOnSave && !angularDestinationCompatible) {
            m_angularValidationError = "El destino actual no admite giro. Desactive explícitamente el giro o corrija el destino/tipo.";
        } else if (m_angularMotionEnabled) {
            validateAngularMotionWithModelContract(
                m_angularVelocity, m_rotationCenter, m_angularValueType,
                m_angularAmplitudeFactor, m_angularAmplitudeTime,
                m_angularAmplitudeValue, m_targetId, m_angularValidationError);
        }
        const bool proposedAngularRoute = m_angularMotionEnabled;
        bool angularRouteInModel = proposedAngularRoute;
        for (int i = 0; i < model->getBCCount(); ++i) {
            BoundaryCondition *other = model->getBC(i);
            if (other == nullptr || other == m_editTarget)
                continue;
            if (other->hasAngularMotion())
                angularRouteInModel = true;
        }
        if (m_angularValidationError.empty() && angularRouteInModel &&
            isBoundary && bcType == 0 &&
            !finiteLinearValues(m_vel, m_valueType, m_amplitudeFactor,
                                m_amplitudeTime, m_amplitudeValue)) {
            m_angularValidationError = "La velocidad, el factor y los valores de amplitud lineales deben ser finitos cuando se usa la ruta angular.";
        }
        if (m_angularValidationError.empty() && angularRouteInModel &&
            angularDestinationCompatible && isBoundary && bcType == 0 && m_valueType == 1 &&
            !angularTableIsValid(m_amplitudeTime, m_amplitudeValue, m_angularValidationError)) {
            m_angularValidationError = "La tabla lineal usada junto con el giro requiere dos o más filas válidas, tiempos no negativos y estrictamente crecientes.";
        }
        if (m_angularValidationError.empty() && angularRouteInModel && isBoundary) {
            for (int p = 0; p < model->getPartCount(); ++p) {
                Part *part = model->getPart(p);
                if (part == nullptr || part->getType() != Rigid)
                    continue;
                int velocityCount = 0;
                for (int i = 0; i < model->getBCCount(); ++i) {
                    BoundaryCondition *other = model->getBC(i);
                    if (other == nullptr || other == m_editTarget ||
                        other->getApplyTo() != ApplyToPart || other->getTargetId() != part->getID() ||
                        other->getType() != VelocityBC)
                        continue;
                    ++velocityCount;
                }
                if (isBoundary && bcType == 0 && m_applyTo == 0 && m_targetId == part->getID())
                    ++velocityCount;
                if (velocityCount > 1) {
                    m_angularValidationError = "La ruta angular requiere una sola BC Velocity por Part rígida; consolide sus movimientos.";
                    break;
                }
            }
        }
        if (m_angularValidationError.empty()) {
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

            if (bc != nullptr && m_angularMotionEnabled) {
                bc->setAngularMotionEnabled(true);
                bc->setAngularVelocity(m_angularVelocity);
                bc->setRotationCenter(m_rotationCenter);
                bc->setAngularValueType(m_angularValueType);
                bc->setAngularAmplitudeFactor(m_angularAmplitudeFactor);
                bc->setAngularAmplitudeTable(
                    m_angularValueType == 1 ? m_angularAmplitudeTime : std::vector<double>(),
                    m_angularValueType == 1 ? m_angularAmplitudeValue : std::vector<double>());
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
            if (bc->kind == ConditionKind::Boundary) {
                BoundaryCondition *boundary = static_cast<BoundaryCondition *>(bc);
                boundary->setAngularMotionEnabled(m_angularMotionEnabled);
                if (m_angularMotionEnabled) {
                    boundary->setAngularVelocity(m_angularVelocity);
                    boundary->setRotationCenter(m_rotationCenter);
                    boundary->setAngularValueType(m_angularValueType);
                    boundary->setAngularAmplitudeFactor(m_angularAmplitudeFactor);
                    boundary->setAngularAmplitudeTable(
                        m_angularValueType == 1 ? m_angularAmplitudeTime : std::vector<double>(),
                        m_angularValueType == 1 ? m_angularAmplitudeValue : std::vector<double>());
                }
            }
        }

        std::cout << (create ? "Created" : "Updated") << " BC\n";
        model->markDirty();
        *p_open = false;
        *sel_bc = nullptr;
        }
    }

    // Cancel
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
        *p_open = false;
    }

    if (!(*p_open)) {
        resetDialogState();
        *sel_bc = nullptr;
        cout << "closed. sel_çbc "<<*sel_bc<<endl;
    }

    ImGui::End();
}
