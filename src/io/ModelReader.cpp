#include "ModelReader.h"
#include "Model.h"
#include "Part.h"
#include "Material.h"
#include "Geom.h"
#include "json_io.h"
#include "gmsh.h"
#include "BoundaryCondition.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>

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

    // =============================================================
    // Configuration
    // =============================================================
    if (j.contains("Configuration")) {
        const auto& conf = j["Configuration"];

        if (conf.contains("modelType"))
            cout << "  Model type: " << conf["modelType"].get<std::string>() << endl;

        if (conf.contains("solver"))
            cout << "  Solver: " << conf["solver"].get<std::string>() << endl;

        if (conf.contains("SPH") && conf["SPH"].contains("hFactor"))
            cout << "  SPH hFactor: " << conf["SPH"]["hFactor"].get<double>() << endl;
        
        if (conf.contains("thermal"))
          m_model->m_thermal_coupling = true;
        else
          m_model->m_thermal_coupling = false;
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
                    cout << "    Geometry source: " << src << endl;
                    geom = new Geom(src);
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
                cout << "    Mesh file: " << meshname << endl;

                if (!fileExists(meshname)) {
                    cerr << "    [Warning] Mesh file not found: " << meshname << endl;
                } else {
                    gmsh::clear();
                    gmsh::open(meshname);
                    gmsh::model::occ::synchronize();
                    part->generateMesh();
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
