#include "ModelReader.h"
#include "Model.h"
#include "Part.h"
#include "Material.h"
#include "Geom.h"
#include "json_io.h"
#include "gmsh.h"
#include "BoundaryCondition.h"
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

    // =============================================================
    // Materials
    // =============================================================
    if (j.contains("Materials")) {
        const auto& mat = j["Materials"];

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
        }

        // Plastic rule (if applicable)
        if (mat.contains("type")) {
            std::string type = mat["type"];
            if (type != "Elastic") {
                cout << "  → Nonlinear material: " << type << endl;
                Plastic_* plRule = nullptr;

                if (type == "Hollomon" && mat.contains("const") && mat["const"].is_array()) {
                    double K = mat["const"][0];
                    double n = mat["const"][1];
                    plRule = new Hollomon(K, n);
                    cout << "    Hollomon(K=" << K << ", n=" << n << ")" << endl;
                }

                if (plRule) {
                    material->m_plastic = plRule->clone();
                    material->m_isplastic = true;
                }
            }
        
            if (m_model->m_thermal_coupling){
              material->k_T = mat["thermalCond"];  
              material->cp_T = mat["thermalHeatCap"];        
            }
            
                    
        }

        m_model->addMaterial(material);
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
                    geom->LoadSTEP(geom_path.string(), origin.x, origin.y, origin.z);
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

            if (jstep.contains("meshing")) {
                const auto &meshing = jstep["meshing"];
                step->m_meshingDebug = meshing.value("debug", true);
                step->m_maxElemAngle = meshing.value("maxElemAngle", 150.0);
                step->m_minElemAngle = meshing.value("minElemAngle", 30.0);
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
            BCApplyTo applyTo = (applyToStr == "Nodes") ? ApplyToNodes : ApplyToPart;

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

            // --- Crear BC ---
            BoundaryCondition* bc = nullptr;

            if (bcType == SymmetryBC) {
                bc = new BoundaryCondition(bcType, applyTo, targetId, normal);
            } else {
                bc = new BoundaryCondition(bcType, applyTo, targetId, val);
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




    cout << "[ModelReader] Model successfully loaded.\n";
    return true;
}
