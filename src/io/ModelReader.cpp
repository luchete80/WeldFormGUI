#include "ModelReader.h"
#include "Model.h"
#include "Part.h"
#include "Material.h"
#include "Section.h"
#include "Geom.h"
#include "Node.h"
#include "json_io.h"
#include "gmsh.h"
#include "BoundaryCondition.h"
#include "InitialCondition.h"
#include "Step.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <filesystem>

using std::cout;
using std::cerr;
using std::endl;

// =============================================================
// Utility: Check if file exists
// =============================================================
static bool fileExists(const std::string& name) {
    std::ifstream f(name.c_str());
    return f.good();
}

// =============================================================
// Constructors
// =============================================================
ModelReader::ModelReader(Model *model) : m_model(model) {}

ModelReader::ModelReader(const char *fname) {
    (void)fname; // Placeholder if needed later
}

// =============================================================
// Main readFromFile function
// =============================================================
bool ModelReader::readFromFile(const std::string& fname) {
    namespace fs = std::filesystem;

    // --- 1. Abrir archivo ---
    std::ifstream i(fname);
    if (!i.is_open()) {
        cerr << "[ModelReader] Error: cannot open file: " << fname << endl;
        return false;
    }

    // --- 2. Parsear JSON ---
    json j;
    try {
        i >> j;
    } catch (const std::exception& e) {
        cerr << "[ModelReader] JSON parse error: " << e.what() << endl;
        return false;
    }

    if (!m_model) {
        cerr << "[ModelReader] Error: model pointer is null." << endl;
        return false;
    }

    cout << "[ModelReader] Reading model from: " << fname << endl;
    m_model->setFilePath(fname);

    // =============================================================
    // Configuration
    // =============================================================
    if (j.contains("Configuration")) {
        const auto& conf = j["Configuration"];

        if (conf.contains("modelType"))
            cout << "  Model type: " << conf["modelType"].get<std::string>() << endl;

        if (conf.contains("solver"))
            cout << "  Solver: " << conf["solver"].get<std::string>() << endl;

        if (conf.contains("analysisType")) {
            std::string analysisType = conf["analysisType"].get<std::string>();
            cout << "  Analysis type: " << analysisType << endl;

            if (analysisType == "PlaneStress2D")
                m_model->setAnalysisType(PlaneStress2D);
            else if (analysisType == "PlaneStrain2D")
                m_model->setAnalysisType(PlaneStrain2D);
            else if (analysisType == "Axisymmetric2D")
                m_model->setAnalysisType(Axisymmetric2D);
            else
                m_model->setAnalysisType(Solid3D);
        }

        if (conf.contains("SPH") && conf["SPH"].contains("hFactor"))
            cout << "  SPH hFactor: " << conf["SPH"]["hFactor"].get<double>() << endl;
        
        if (conf.contains("thermal"))
          m_model->m_thermal_coupling = true;
        else
          m_model->m_thermal_coupling = false;
    }

    if (j.contains("Contact") && j["Contact"].is_array() && !j["Contact"].empty()) {
        const auto &contact = j["Contact"][0];
        ContactProperties &props = m_model->contactProps();
        props.fricCoeffStatic = contact.value("fricCoeffStatic", props.fricCoeffStatic);
        props.gapPenaltyScale = contact.value("gapPenaltyScale", props.gapPenaltyScale);
        props.heatCondCoeff = contact.value("heatCondCoeff", props.heatCondCoeff);
        props.heatConductance = contact.value("heatConductance", props.heatConductance);
        props.maxAccel = contact.value("maxAccel", props.maxAccel);
        props.maxPenetRatio = contact.value("maxPenetRatio", props.maxPenetRatio);
        props.penaltyFactor = contact.value("penaltyFactor", props.penaltyFactor);
        props.useGapPenalty = contact.value("useGapPenalty", props.useGapPenalty);
    }

    if (j.contains("Remeshing") && j["Remeshing"].is_object()) {
        const auto &remeshing = j["Remeshing"];
        RemeshingSettings &settings = m_model->remeshing();
        settings.enabled = remeshing.value("enabled", settings.enabled);
        settings.minStrain = remeshing.value("minStrain", settings.minStrain);
        settings.maxStrain = remeshing.value("maxStrain", settings.maxStrain);
        settings.mapVel = remeshing.value("mapVel", settings.mapVel);
        settings.mapAcc = remeshing.value("mapAcc", settings.mapAcc);
        settings.maxCount = remeshing.value("maxCount", settings.maxCount);
        settings.dampFactor = remeshing.value("dampFactor", settings.dampFactor);
        settings.minFrac = remeshing.value("minFrac", settings.minFrac);
        settings.maxFrac = remeshing.value("maxFrac", settings.maxFrac);
        settings.epsRef = remeshing.value("epsRef", settings.epsRef);
        settings.beta = remeshing.value("beta", settings.beta);
        settings.type = remeshing.value("type", settings.type);
        settings.debug = remeshing.value("debug", settings.debug);
        settings.minElemAngle = remeshing.value("minElemAngle", settings.minElemAngle);
        settings.maxElemAngle = remeshing.value("maxElemAngle", settings.maxElemAngle);
        settings.transitionAngle = remeshing.value("transitionAngle", settings.transitionAngle);
    }

    if (j.contains("SymmetryPlanes") && j["SymmetryPlanes"].is_array()) {
        for (const auto& jplane : j["SymmetryPlanes"]) {
            SymmetryPlane plane;
            plane.enabled = jplane.value("enabled", true);
            plane.axis = jplane.value("axis", 0);
            plane.value = jplane.value("value", 0.0);
            m_model->upsertSymmetryPlane(plane);
        }
    }

    // =============================================================
    // Materials
    // =============================================================
    if (j.contains("Materials")) {
        const auto* mat_ptr = &j["Materials"];
        if (j["Materials"].is_array()) {
            if (j["Materials"].empty()) {
                mat_ptr = nullptr;
            } else {
                mat_ptr = &j["Materials"][0];
            }
        }
        if (mat_ptr == nullptr) {
            return false;
        }
        const auto& mat = *mat_ptr;

        double rho = mat.value("density0", 0.0);
        double E   = mat.value("youngsModulus", 0.0);
        double nu  = mat.value("poissonsRatio", 0.0);

        cout << "[ModelReader] Material constants: E=" << E << ", ν=" << nu << ", ρ=" << rho << endl;

        Elastic_ elastic(E, nu);
        Material_* material = new Material_(elastic);
        material->setDensityConstant(rho);
        material->yieldStress0 = mat.value("yieldStress0", material->yieldStress0);
        if (mat.contains("strRange") && mat["strRange"].is_array() && mat["strRange"].size() >= 2) {
            material->strRange = {mat["strRange"][0].get<double>(), mat["strRange"][1].get<double>()};
            material->e_min = material->strRange[0];
            material->e_max = material->strRange[1];
        }
        if (mat.contains("strdotRange") && mat["strdotRange"].is_array() && mat["strdotRange"].size() >= 2) {
            material->er_min = mat["strdotRange"][0].get<double>();
            material->er_max = mat["strdotRange"][1].get<double>();
        }
        if (mat.contains("tempRange") && mat["tempRange"].is_array() && mat["tempRange"].size() >= 2) {
            material->T_min = mat["tempRange"][0].get<double>();
            material->T_max = mat["tempRange"][1].get<double>();
        }
        material->tabulated_export_csv_reference = mat.contains("flowStressCsv");

        // Plastic rule (if applicable)
        if (mat.contains("type")) {
            std::string type = mat["type"];
            if (type != "Elastic") {
                cout << "  → Nonlinear material: " << type << endl;
                Plastic_* plRule = nullptr;

                if (type == "Bilinear" && mat.contains("const") && mat["const"].is_array() && mat["const"].size() >= 2) {
                    double sy0 = mat["const"][0];
                    double Et = mat["const"][1];
                    plRule = new Bilinear(sy0, Et);
                    cout << "    Bilinear(sy0=" << sy0 << ", Et=" << Et << ")" << endl;
                }

                if (type == "Hollomon" && mat.contains("const") && mat["const"].is_array()) {
                    double K = mat["const"][0];
                    double n = mat["const"][1];
                    plRule = new Hollomon(K, n);
                    material->InitHollomon(elastic, material->yieldStress0, K, n);
                    cout << "    Hollomon(K=" << K << ", n=" << n << ")" << endl;
                }

                if (type == "JohnsonCook" && mat.contains("const") && mat["const"].is_array() && mat["const"].size() >= 7) {
                    double B = mat["const"][0];
                    double n = mat["const"][1];
                    double C = mat["const"][2];
                    double eps0 = mat["const"][3];
                    double m = mat["const"][4];
                    double Tm = mat["const"][5];
                    double Tt = mat["const"][6];
                    plRule = new JohnsonCook(B, n, C, eps0, m, Tm, Tt, material->yieldStress0);
                    material->Init_JohnsonCook(elastic, material->yieldStress0, B, n, C, eps0, m, Tm, Tt);
                    cout << "    JohnsonCook(B=" << B << ", n=" << n << ", C=" << C << ")" << endl;
                }

                if (type == "GMT" && mat.contains("const") && mat["const"].is_array() && mat["const"].size() >= 8) {
                    double n1 = mat["const"][0];
                    double n2 = mat["const"][1];
                    double C1 = mat["const"][2];
                    double C2 = mat["const"][3];
                    double m1 = mat["const"][4];
                    double m2 = mat["const"][5];
                    double I1 = mat["const"][6];
                    double I2 = mat["const"][7];
                    plRule = new GMT(n1, n2, C1, C2, m1, m2, I1, I2,
                                     material->e_min, material->e_max,
                                     material->er_min, material->er_max,
                                     material->T_min, material->T_max);
                    material->Init_GMT(elastic, n1, n2, C1, C2, m1, m2, I1, I2,
                                       material->e_min, material->e_max,
                                       material->er_min, material->er_max,
                                       material->T_min, material->T_max);
                    cout << "    GMT(n1=" << n1 << ", n2=" << n2 << ", C1=" << C1 << ", C2=" << C2 << ")" << endl;
                }

                if (type == "Tabulated" && mat.contains("flowStressTable")) {
                    const auto &table = mat["flowStressTable"];
                    if (table.contains("strainGrid") && table.contains("strainRateGrid") &&
                        table.contains("temperatureGrid") && table.contains("stressValues")) {
                        material->tabulated_enabled = true;
                        material->tabulatedStrainGrid = table["strainGrid"].get<std::vector<double>>();
                        material->tabulatedRateGrid = table["strainRateGrid"].get<std::vector<double>>();
                        material->tabulatedTemperatureGrid = table["temperatureGrid"].get<std::vector<double>>();
                        material->tabulatedStressValues = table["stressValues"].get<std::vector<double>>();
                        if (table.contains("csvFile")) {
                            material->tableCsvPath = table["csvFile"].get<std::string>();
                        }
                        material->Material_model = TABULATED;
                        if (!material->tabulatedStrainGrid.empty()) {
                            material->e_min = material->tabulatedStrainGrid.front();
                            material->e_max = material->tabulatedStrainGrid.back();
                            material->strRange = {material->e_min, material->e_max};
                        }
                        if (!material->tabulatedRateGrid.empty()) {
                            material->er_min = material->tabulatedRateGrid.front();
                            material->er_max = material->tabulatedRateGrid.back();
                        }
                        if (!material->tabulatedTemperatureGrid.empty()) {
                            material->T_min = material->tabulatedTemperatureGrid.front();
                            material->T_max = material->tabulatedTemperatureGrid.back();
                        }
                        plRule = new Tabulated();
                        cout << "    Tabulated flow-stress family loaded: "
                             << material->tabulatedStrainGrid.size() << "x"
                             << material->tabulatedRateGrid.size() << "x"
                             << material->tabulatedTemperatureGrid.size() << endl;
                    }
                }
                if (type == "Tabulated" && mat.contains("flowStressCsv")) {
                    material->tableCsvPath = mat["flowStressCsv"].get<std::string>();
                    material->tabulated_export_csv_reference = true;
                }

                if (plRule) {
                    material->m_plastic = plRule->clone();
                    material->m_isplastic = true;
                    delete plRule;
                }
            }
        
            if (m_model->m_thermal_coupling){
              material->k_T = mat.value("thermalCond", 0.0);  
              material->cp_T = mat.value("thermalHeatCap", 0.0);        
            }
            
                    
        }

        m_model->addMaterial(material);
    }

    // =============================================================
    // Sections
    // =============================================================
    if (j.contains("Sections") && j["Sections"].is_array()) {
        cout << "[ModelReader] Reading " << j["Sections"].size() << " section(s)" << endl;
        for (const auto& jsection : j["Sections"]) {
            Section* section = new Section();
            section->setId(jsection.value("id", -1));
            section->setName(jsection.value("name", std::string()));
            section->setMaterialIndex(jsection.value("materialIndex", -1));
            section->setIntendedElementType(jsection.value("intendedElementType", std::string("Auto")));
            section->setThickness(jsection.value("thickness", 1.0));
            m_model->addSection(section);
        }
    }

    // =============================================================
    // Parts
    // =============================================================
    if (j.contains("model") && j["model"].contains("parts")) {
        const auto& parts = j["model"]["parts"];
        cout << "[ModelReader] Reading " << parts.size() << " part(s)" << endl;

        int idx = 0;
        for (const auto& jpart : parts) {
            cout << "  → Part " << idx << endl;

            Part* part = nullptr;
            Geom* geom = nullptr;

            // --- Geometry ---
            if (jpart.contains("geometry")) {
                const auto& geo = jpart["geometry"];
                std::string src = geo.value("source", "");
                double3 origin = make_double3(0.0, 0.0, 0.0);
                readVector(geo["origin"], origin);

                if (!src.empty()) {
                    fs::path geom_path = fs::path(src);
                    if (geom_path.is_relative())
                        geom_path = fs::path(fname).parent_path() / geom_path;
                    cout << "    Geometry source: " << src << endl;
                    geom = new Geom();
                    geom->LoadSTEP(geom_path.string());
                    geom->setOrigin(origin);
                    part = new Part(geom);
                }
            }

            if (!part)
                part = new Part();

            // --- Name & ID ---
            if (jpart.contains("name")) {
                std::string pname = jpart["name"];
                part->setName(pname.c_str());
                cout << "    Name: " << pname << endl;
            }

            if (jpart.contains("id"))
                part->setId(jpart["id"].get<int>());
            if (jpart.contains("sectionId"))
                part->setSectionId(jpart["sectionId"].get<int>());

            // --- Mesh ---
            if (jpart.contains("mesh") && jpart["mesh"].contains("source")) {
                std::string meshname = jpart["mesh"]["source"].get<std::string>();
                fs::path mesh_path = fs::path(meshname);
                if (mesh_path.is_relative())
                    mesh_path = fs::path(fname).parent_path() / mesh_path;
                cout << "    Mesh file: " << meshname << endl;

                if (!fileExists(mesh_path.string())) {
                    cerr << "    [Warning] Mesh file not found: " << mesh_path.string() << endl;
                } else {
                    std::string ext = mesh_path.extension().string();
                    for (auto &c : ext) c = std::tolower(c);

                    if (ext == ".bdf") {
                        part->generateMeshFromNastranFile(mesh_path.string());
                    } else {
                        gmsh::clear();
                        gmsh::open(mesh_path.string());
                        gmsh::model::occ::synchronize();
                        part->generateMesh();
                    }
                }
            }

            // --- Type (rigid/deformable) ---
            bool isRigid = jpart.value("isRigid", false);
            part->setType(isRigid ? 1 : 0);
            cout << "    Type: " << (isRigid ? "Rigid" : "Deformable") << endl;

            // Add to model
            m_model->addPart(part);
            idx++;
        }
    }//parts

    // =============================================================
    // Node Sets
    // =============================================================
    if (j.contains("NodeSets") && j["NodeSets"].is_array()) {
        cout << "[ModelReader] Reading " << j["NodeSets"].size() << " node set(s)" << endl;

        for (const auto& jset : j["NodeSets"]) {
            const int partId = jset.value("partId", -1);
            const int setId = jset.value("id", -1);
            const std::string label = jset.value("label", "");

            Part* targetPart = nullptr;
            for (int i = 0; i < m_model->getPartCount(); ++i) {
                Part* part = m_model->getPart(i);
                if (part != nullptr && part->getId() == partId) {
                    targetPart = part;
                    break;
                }
            }

            if (targetPart == nullptr || targetPart->getMesh() == nullptr) {
                cerr << "  [Warning] NodeSet " << setId
                     << " references missing partId=" << partId << endl;
                continue;
            }

            Mesh* mesh = targetPart->getMesh();
            NodeSet nodeSet(mesh);
            nodeSet.setEntityId(setId);
            nodeSet.setLabel(label);

            if (jset.contains("nodeIds") && jset["nodeIds"].is_array()) {
                for (const auto& jnodeId : jset["nodeIds"]) {
                    const int nodeId = jnodeId.get<int>();
                    Node* targetNode = nullptr;

                    for (int n = 0; n < mesh->getNodeCount(); ++n) {
                        Node* node = mesh->getNode(n);
                        if (node != nullptr && node->getId() == nodeId) {
                            targetNode = node;
                            break;
                        }
                    }

                    if (targetNode != nullptr) {
                        nodeSet.add(targetNode);
                    } else {
                        cerr << "  [Warning] NodeSet " << setId
                             << " references missing nodeId=" << nodeId << endl;
                    }
                }
            }

            mesh->addNodeSet(nodeSet);
            cout << "  -> NodeSet id=" << setId
                 << " label=\"" << label << "\""
                 << " nodes=" << nodeSet.getItemCount()
                 << " partId=" << partId << endl;
        }
    }

    if (j.contains("ElementSets") && j["ElementSets"].is_array()) {
        cout << "[ModelReader] Reading " << j["ElementSets"].size() << " element set(s)" << endl;

        for (const auto& jset : j["ElementSets"]) {
            const int partId = jset.value("partId", -1);
            const int setId = jset.value("id", -1);
            const std::string label = jset.value("label", "");

            Part* targetPart = nullptr;
            for (int i = 0; i < m_model->getPartCount(); ++i) {
                Part* part = m_model->getPart(i);
                if (part != nullptr && part->getId() == partId) {
                    targetPart = part;
                    break;
                }
            }

            if (targetPart == nullptr || targetPart->getMesh() == nullptr) {
                cerr << "  [Warning] ElementSet " << setId
                     << " references missing partId=" << partId << endl;
                continue;
            }

            Mesh* mesh = targetPart->getMesh();
            ElementSet elementSet(mesh);
            elementSet.setEntityId(setId);
            elementSet.setLabel(label);

            if (jset.contains("elementIds") && jset["elementIds"].is_array()) {
                for (const auto& jelementId : jset["elementIds"]) {
                    const int elementId = jelementId.get<int>();
                    Element* targetElement = nullptr;

                    for (int e = 0; e < mesh->getElemCount(); ++e) {
                        Element* element = mesh->getElem(e);
                        if (element != nullptr && element->getId() == elementId) {
                            targetElement = element;
                            break;
                        }
                    }

                    if (targetElement != nullptr) {
                        elementSet.add(targetElement);
                    } else {
                        cerr << "  [Warning] ElementSet " << setId
                             << " references missing elementId=" << elementId << endl;
                    }
                }
            }

            mesh->addElementSet(elementSet);
            cout << "  -> ElementSet id=" << setId
                 << " label=\"" << label << "\""
                 << " elements=" << elementSet.getItemCount()
                 << " partId=" << partId << endl;
        }
    }

    if (j.contains("FaceSets") && j["FaceSets"].is_array()) {
        cout << "[ModelReader] Reading " << j["FaceSets"].size() << " face set(s)" << endl;

        for (const auto& jset : j["FaceSets"]) {
            const int partId = jset.value("partId", -1);
            const int setId = jset.value("id", -1);
            const std::string label = jset.value("label", "");

            Part* targetPart = nullptr;
            for (int i = 0; i < m_model->getPartCount(); ++i) {
                Part* part = m_model->getPart(i);
                if (part != nullptr && part->getId() == partId) {
                    targetPart = part;
                    break;
                }
            }

            if (targetPart == nullptr || targetPart->getMesh() == nullptr) {
                cerr << "  [Warning] FaceSet " << setId
                     << " references missing partId=" << partId << endl;
                continue;
            }

            Mesh* mesh = targetPart->getMesh();
            FaceSet faceSet(mesh);
            faceSet.setEntityId(setId);
            faceSet.setLabel(label);

            if (jset.contains("faces") && jset["faces"].is_array()) {
                for (const auto& jface : jset["faces"]) {
                    Face face;
                    face.setEntityId(jface.value("id", faceSet.getItemCount()));
                    face.setOwnerElementId(jface.value("ownerElementId", -1));
                    face.setLocalFaceIndex(jface.value("localFaceIndex", -1));

                    std::vector<int> nodeIds;
                    if (jface.contains("nodeIds") && jface["nodeIds"].is_array()) {
                        for (const auto& jnodeId : jface["nodeIds"]) {
                            nodeIds.push_back(jnodeId.get<int>());
                        }
                    }
                    face.setNodeIds(nodeIds);
                    faceSet.add(face);
                }
            }

            mesh->addFaceSet(faceSet);
            cout << "  -> FaceSet id=" << setId
                 << " label=\"" << label << "\""
                 << " faces=" << faceSet.getItemCount()
                 << " partId=" << partId << endl;
        }
    }

    // =============================================================
    // Boundary Conditions
    // =============================================================
    if (j.contains("Steps") && j["Steps"].is_array()) {
        for (const auto& jstep : j["Steps"]) {
            Step *step = new Step();
            if (jstep.contains("id")) {
                int step_id = jstep["id"].get<int>();
                step->setId(step_id);
            }
            if (jstep.contains("name"))
                step->setName(jstep["name"].get<std::string>().c_str());

            std::string stepType = jstep.value("type", "Explicit");
            step->setStepType(stepType == "Implicit" ? ImplicitStep : ExplicitStep);

            step->m_nproc = jstep.value("nproc", 1);
            step->m_cflFactor = jstep.value("cflFactor", 0.3);
            if (jstep.contains("autoTS") && jstep["autoTS"].is_array() && jstep["autoTS"].size() == 3) {
                for (int k = 0; k < 3; ++k)
                    step->m_autoTS[k] = jstep["autoTS"][k].get<bool>();
            }
            step->m_kernelGradCorr = jstep.value("kernelGradCorr", false);
            step->m_simTime = jstep.value("simTime", 200.0);
            step->m_artifViscAlpha = jstep.value("artifViscAlpha", 1.0);
            step->m_artifViscBeta = jstep.value("artifViscBeta", 0.0);
            step->m_outTime = jstep.value("outTime", 1.0);
            step->m_fixedTS = jstep.value("fixedTS", false);
            step->m_axiSymmVol = jstep.value("axiSymmVol", false);
            step->m_elemLengthFraction = jstep.value("elemLengthFraction", 0.2);

            if (jstep.contains("meshing") && !j.contains("Remeshing")) {
                const auto &meshing = jstep["meshing"];
                RemeshingSettings &settings = m_model->remeshing();
                settings.debug = meshing.value("debug", settings.debug);
                settings.maxElemAngle = meshing.value("maxElemAngle", settings.maxElemAngle);
                settings.minElemAngle = meshing.value("minElemAngle", settings.minElemAngle);
            }

            if (jstep.contains("implicit")) {
                const auto &implicit = jstep["implicit"];
                step->m_implicitType = implicit.value("type", "Picard");
                step->m_velTol = implicit.value("velTol", 5e-2);
                step->m_pressTol = implicit.value("pressTol", 10.0);
                step->m_forceTol = implicit.value("forceTol", 10.0);
                step->m_divTol = implicit.value("divTol", 1.0);
                step->m_omegaV = implicit.value("omegaV", 0.4);
                step->m_omegaP = implicit.value("omegaP", 0.1);
                step->m_maxIter = implicit.value("maxIter", 200);
                step->m_timeStepGrowthFactor = implicit.value("timeStepGrowthFactor", 1.2);
                step->m_useWeakSprings = implicit.value("useSprings", false);
                step->m_springFactor = implicit.value("springFactor", 1.0e-7);
                step->m_springStiffness = implicit.value("springStiffness", 0.0);
                step->m_springMode = implicit.value("springMode", 1);
            }

            m_model->addStep(step);
        }
    }

    if (j.contains("BoundaryConditions") && j["BoundaryConditions"].is_array()) {
        const auto& bcArray = j["BoundaryConditions"];
        cout << "[ModelReader] Reading " << bcArray.size() << " Boundary Condition(s)" << endl;

        for (const auto& jbc : bcArray) {

            // --- Tipo de BC ---
            std::string typeStr = jbc.value("type", "VelocityBC");
            BCType bcType = VelocityBC;

            if (typeStr == "VelocityBC")        bcType = VelocityBC;
            else if (typeStr == "DisplacementBC") bcType = DisplacementBC;
            else if (typeStr == "SymmetryBC")     bcType = SymmetryBC;
            else {
                cerr << "  [Warning] Unknown BC type: " << typeStr << " → defaulting to VelocityBC" << endl;
            }

            // --- ApplyTo ---
            std::string applyToStr = jbc.value("applyTo", "Part");
            BCApplyTo applyTo = (applyToStr == "NodeSet" || applyToStr == "Nodes")
              ? ApplyToNodeSet
              : ApplyToPart;

            // --- Target ID ---
            int targetId = jbc.value("targetId", -1);
            if (targetId < 0) {
                cerr << "  [Warning] BoundaryCondition missing targetId, skipping." << endl;
                continue;
            }

            // --- Valor para Velocity/Displacement ---
            double3 val = make_double3(0.0, 0.0, 0.0);
            if ((bcType == VelocityBC || bcType == DisplacementBC) &&
                jbc.contains("value") &&
                jbc["value"].is_array() &&
                jbc["value"].size() == 3) 
            {
                val.x = jbc["value"][0];
                val.y = jbc["value"][1];
                val.z = jbc["value"][2];
            }

            bool dofMask[3] = {true, true, true};
            if ((bcType == VelocityBC || bcType == DisplacementBC) &&
                jbc.contains("dofMask") &&
                jbc["dofMask"].is_array() &&
                jbc["dofMask"].size() == 3)
            {
                dofMask[0] = jbc["dofMask"][0].get<bool>();
                dofMask[1] = jbc["dofMask"][1].get<bool>();
                dofMask[2] = jbc["dofMask"][2].get<bool>();
            }

            // --- Normal para SymmetryBC ---
            double3 normal = make_double3(0.0, 0.0, 1.0);
            if (bcType == SymmetryBC &&
                jbc.contains("normal") &&
                jbc["normal"].is_array() &&
                jbc["normal"].size() == 3) 
            {
                normal.x = jbc["normal"][0];
                normal.y = jbc["normal"][1];
                normal.z = jbc["normal"][2];

            }

            ConditionValueType valueType = ConstantValue;
            if (jbc.contains("valueType")) {
                valueType = (jbc["valueType"].get<int>() == static_cast<int>(AmplitudeValue))
                  ? AmplitudeValue
                  : ConstantValue;
            }
            const double amplitudeFactor = jbc.value("amplitudeFactor", 1.0);
            std::vector<double> amplitudeTime;
            std::vector<double> amplitudeValue;
            if (jbc.contains("amplitudeTime") && jbc["amplitudeTime"].is_array())
                amplitudeTime = jbc["amplitudeTime"].get<std::vector<double>>();
            if (jbc.contains("amplitudeValue") && jbc["amplitudeValue"].is_array())
                amplitudeValue = jbc["amplitudeValue"].get<std::vector<double>>();

            // --- Crear BC ---
            BoundaryCondition* bc = nullptr;

            if (bcType == SymmetryBC) {
                bc = new BoundaryCondition(bcType, applyTo, targetId, normal);
            } else {
                bc = new BoundaryCondition(bcType, applyTo, targetId, val);
                bc->setValueType(valueType);
                bc->setAmplitudeFactor(amplitudeFactor);
                bc->setAmplitudeTable(amplitudeTime, amplitudeValue);
                bc->setDofMask(dofMask[0], dofMask[1], dofMask[2]);
            }

            m_model->addBoundaryCondition(bc);

            cout << "  → Added BC: "
                 << typeStr
                 << ", ApplyTo=" << applyToStr
                 << ", TargetID=" << targetId;

            if (bcType == SymmetryBC)
                cout << ", Normal=(" << normal.x << ", " << normal.y << ", " << normal.z << ")";
            else
                cout << ", Value=(" << val.x << ", " << val.y << ", " << val.z << ")";

            cout << endl;
        }

        cout << "[ModelReader] Done reading Boundary Conditions." << endl;
    }

    if (j.contains("InitialConditions") && j["InitialConditions"].is_array()) {
        const auto& icArray = j["InitialConditions"];
        cout << "[ModelReader] Reading " << icArray.size() << " Initial Condition(s)" << endl;

        for (const auto& jic : icArray) {
            std::string typeStr = jic.value("type", "VelocityIC");
            BCType icType = VelocityIC;
            if (typeStr == "TempIC")
                icType = TempIC;

            std::string applyToStr = jic.value("applyTo", "Part");
            BCApplyTo applyTo = (applyToStr == "NodeSet" || applyToStr == "Nodes")
              ? ApplyToNodeSet
              : ApplyToPart;

            int targetId = jic.value("targetId", -1);
            if (targetId < 0) {
                cerr << "  [Warning] InitialCondition missing targetId, skipping." << endl;
                continue;
            }

            double3 val = make_double3(0.0, 0.0, 0.0);
            if (jic.contains("value") && jic["value"].is_array() && jic["value"].size() == 3) {
                val.x = jic["value"][0];
                val.y = jic["value"][1];
                val.z = jic["value"][2];
            }

            InitialCondition* ic = new InitialCondition(icType, applyTo, targetId, val);
            const ConditionValueType valueType =
              (jic.value("valueType", static_cast<int>(ConstantValue)) == static_cast<int>(AmplitudeValue))
                ? AmplitudeValue
                : ConstantValue;
            ic->setValueType(valueType);
            ic->setAmplitudeFactor(jic.value("amplitudeFactor", 1.0));
            if (jic.contains("amplitudeTime") && jic["amplitudeTime"].is_array() &&
                jic.contains("amplitudeValue") && jic["amplitudeValue"].is_array()) {
                ic->setAmplitudeTable(jic["amplitudeTime"].get<std::vector<double>>(),
                                      jic["amplitudeValue"].get<std::vector<double>>());
            }
            if (jic.contains("dofMask") && jic["dofMask"].is_array() && jic["dofMask"].size() == 3) {
                ic->setDofMask(jic["dofMask"][0].get<bool>(),
                               jic["dofMask"][1].get<bool>(),
                               jic["dofMask"][2].get<bool>());
            }
            m_model->addInitialCondition(ic);
        }

        cout << "[ModelReader] Done reading Initial Conditions." << endl;
    }




    cout << "[ModelReader] Model successfully loaded.\n";
    return true;
}
