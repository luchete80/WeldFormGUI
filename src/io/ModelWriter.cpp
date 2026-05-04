
#include "ModelWriter.h"
#include "Model.h"
#include "Part.h"
#include <iostream>
#include <fstream>
#include <iomanip> //setw
#include <filesystem>
#include <system_error>
#include "Material.h"
#include "Geom.h"
#include "Node.h"

#include <nlohmann/json.hpp>
#include "json_io.h"
#include "BoundaryCondition.h"
#include "Step.h"

using json = nlohmann::json;

ModelWriter::ModelWriter(Model& model) : m_model(model) {}


//~ void to_json(json& j, const Vec3& v) {
    //~ j = json{v.x, v.y, v.z};
//~ }


void ModelWriter::writeToFile(std::string fname){
  namespace fs = std::filesystem;
  json m_json;
  
  //json j;
	//i >> j;

/*
	nlohmann::json m_config 		= j["Configuration"];
	nlohmann::json material 	= j["Materials"];
	nlohmann::json domblock 	= j["DomainBlocks"];
	nlohmann::json domzones 	= j["DomainZones"];
	nlohmann::json amplitudes 	= j["Amplitudes"];
	nlohmann::json rigbodies 		= j["RigidBodies"];
  nlohmann::json contact_ 		= j["Contact"];
	nlohmann::json bcs 			= j["BoundaryConditions"];
	nlohmann::json ics 			= j["InitialConditions"];
*/
  //std::ofstream o(m_model.getName());
  std::ofstream o(fname);
  std::string filename = m_model.getName();
  fs::path json_path(fname);
  fs::path json_dir = json_path.has_parent_path() ? json_path.parent_path() : fs::path(".");
  std::error_code mkdir_ec;
  fs::create_directories(json_dir, mkdir_ec);
  if (!m_model.getHasName()) cout << "Not has name!"<<endl;
  std::cout << "Writing to file: " << filename << std::endl;

  //cout << "Exporting Model "<< <<"to json "<<endl;

/*  
  ostringstream oss;
  
  oss << "\"Configuration\":{ \" "            <<endl<<
           "  \"particleRadius\": 0.001,"       <<endl<<      //
           "  \"hFactor\": 1.2,\""              <<endl<<
           "  \"cflMethod\": 1, "               <<endl<< 
           "  \"cflFactor\": 0.4,"              <<endl<<
           "  \"sumType\": \"Nishimura\","      <<endl<<
           "  \"autoTS\": [false,false,false]," <<endl<<
           "  \"simTime\": 5.0e-7,"             <<endl<<
           "  \"artifViscAlpha\": 1.0,"         <<endl<<
           "  \"outTime\": 1.94e-7 "            <<endl<<
           "},"                                 <<endl;
  
  //Can be maintained a Domain Block??
  oss << "\"Materials\":"                        <<endl<< 
         "[{"                                    <<endl<<
         "}],"                                   <<endl;

  oss << "\"DomainBlocks\":"                     <<endl<< 
         "[{"                                    <<endl<<
         "}],"                                   <<endl;

  //Loop trough sets
  oss << "\"DomainZones\":"                      <<endl<< 
         "[{"                                    <<endl<<
         "}],"                                   <<endl;  
  
  oss << "\"Amplitudes\":"                      <<endl<< 
         "[{"                                    <<endl<<
         "}],"                                   <<endl;  
    
  oss << "\"BoundaryConditions\":"               <<endl<< 
         "[{"                                    <<endl<<
         "}],"                                   <<endl;  
  
  oss << "}"                                   <<endl;    

	of << oss.str();
*/  
  cout << "Saving model/s"<<endl;
  
  m_json["Configuration"]["modelType"] = "SPH";
  m_json["Configuration"]["solver"]    = "WeldForm";
  switch (m_model.getAnalysisType()) {
    case PlaneStress2D:
      m_json["Configuration"]["analysisType"] = "PlaneStress2D";
      break;
    case PlaneStrain2D:
      m_json["Configuration"]["analysisType"] = "PlaneStrain2D";
      break;
    case Axisymmetric2D:
      m_json["Configuration"]["analysisType"] = "Axisymmetric2D";
      break;
    case Solid3D:
    default:
      m_json["Configuration"]["analysisType"] = "Solid3D";
      break;
  }
  
  if (m_model.m_thermal_coupling)
    m_json["Configuration"]["thermal"] = true; 

  m_json["Contact"] = json::array();
  m_json["Contact"].push_back({
    {"fricCoeffStatic", m_model.contactProps().fricCoeffStatic},
    {"gapPenaltyScale", m_model.contactProps().gapPenaltyScale},
    {"heatCondCoeff", m_model.contactProps().heatCondCoeff},
    {"heatConductance", m_model.contactProps().heatConductance},
    {"maxAccel", m_model.contactProps().maxAccel},
    {"maxPenetRatio", m_model.contactProps().maxPenetRatio},
    {"penaltyFactor", m_model.contactProps().penaltyFactor},
    {"useGapPenalty", m_model.contactProps().useGapPenalty}
  });
  
  //SHOULD BE AT SPH
  //m_json["Configuration"]["hFactor"] = 1.2;
  m_json["Configuration"]["SPH"]["hFactor"] = 1.2;
  
  if (m_model.getMaterialCount()>0){
    cout << "Writing materials .."<<endl;
    m_json["Materials"]["density0"]      = m_model.getMaterial(0)->getDensityConstant();  
    m_json["Materials"]["poissonsRatio"] = m_model.getMaterial(0)->Elastic().nu();  
    m_json["Materials"]["youngsModulus"] = m_model.getMaterial(0)->Elastic().E();  
    if (m_model.m_thermal_coupling){
      m_json["Materials"]["thermalCond"] = m_model.getMaterial(0)->k_T;  
      m_json["Materials"]["thermalHeatCap"] = m_model.getMaterial(0)->cp_T;        
    }
    
    cout << "Done."<<endl;
  }


  cout << "Loop thorough " <<m_model.getPartCount()<<" parts..."<<endl;
  int i = 0;
  for (std::vector<Part*>::iterator it = m_model.m_part.begin(); it != m_model.m_part.end(); ++it){
   Part* part = *it;

      json jpart;
      jpart["id"] = part->getId();

      //if (!part->getName().empty())
          jpart["name"] = part->getName();
      //else
      //    jpart["name"] = "Part_" + std::to_string(part->getId());
          
      //~ jpart["name"] = part->getName();
      //~ jpart["material"] = part->getMaterialName();

      //~ // Mesh
      //~ jpart["mesh"] = json::object();
      //~ jpart["mesh"]["nodes"] = json::array();
      //~ for (auto& node : part->getNodes()) {
          //~ jpart["mesh"]["nodes"].push_back({node.x, node.y, node.z});
      //~ }
      if (part->getMesh() != nullptr){
      //~ jpart["mesh"]["elements"] = json::array();
      //~ for (auto& elem : part->getElements()) {
          //~ json jelem;
          //~ jelem["id"] = elem.id;
          //~ jelem["type"] = elem.type;
          //~ jelem["connectivity"] = elem.connectivity; // suponiendo que es vector<int>
          //~ jpart["mesh"]["elements"].push_back(jelem);
      //~ }
    }
      if (part->getType() == Rigid){
        jpart["isRigid"] = true;
        cout << "Part "<<i << " is rigid"<<endl;
      } else {
        jpart["isRigid"] = false;
        
        }
      
      //~ else 
        //~ jpart["isRigid"] = false;
      //~ // Geometry (si existe)
      if (part->isGeom()) {
          const std::string step_filename =
              m_model.getName() + "_part_" + std::to_string(part->getId()) + ".step";
          fs::path geom_path = json_dir / step_filename;

          part->getGeom()->setFileName(geom_path.string());
          if (!part->getGeom()->ExportSTEP()) {
            std::cerr << "Error exporting geometry STEP: " << geom_path << std::endl;
          }

          cout << "Part has geom"<<endl;
          jpart["geometry"]["source"] = fs::relative(geom_path, json_dir).string();
          jpart["geometry"]["origin"] = writeVector(part->getGeom()->getOrigin());
          //jpart["geometry"]["representation"] = "BRep";
          //~ jpart["geometry"]["bounding_box"] = {
              //~ {part->getBBoxMinX(), part->getBBoxMinY(), part->getBBoxMinZ()},
              //~ {part->getBBoxMaxX(), part->getBBoxMaxY(), part->getBBoxMaxZ()}
          //~ };

      } 
      if (part->getMesh() != nullptr) {
        cout << "Part "<<i << " is meshed."<<endl;
        const bool use_bdf_mesh = true;
        const std::string meshname =
            m_model.getName() + "_part_" + std::to_string(part->getId()) + ".bdf";
        fs::path mesh_path = json_dir / meshname;
        jpart["mesh"]["source"] = fs::relative(mesh_path, json_dir).string();

        cout << "Exporting current mesh to Nastran..."<<endl;
        part->getMesh()->exportToNASTRAN(mesh_path.string());
        part->setMeshSourceFile(mesh_path.string());

        if (m_model.getPrevName()!=m_model.getName()){
         std::string kname = m_model.getName()+"_part_" + std::to_string(i) + ".k";
            cout << "Exporting to LSDyna..."<<endl;
            part->getMesh()->exportToLSDYNA(kname);
        
        }// IF NAME CHANGED
      } else {
          cout << "Part "<<i << " is not meshed."<<endl;
      }

      m_json["model"]["parts"].push_back(jpart);
      
      i++;
    
  }//PART

  int totalNodeSets = 0;
  for (int partIndex = 0; partIndex < m_model.getPartCount(); ++partIndex) {
    Part* part = m_model.getPart(partIndex);
    if (part == nullptr || part->getMesh() == nullptr) {
      continue;
    }

    Mesh* mesh = part->getMesh();
    for (int setIndex = 0; setIndex < mesh->getNodeSetCount(); ++setIndex) {
      const NodeSet& nodeSet = mesh->getNodeSet(setIndex);

      json jset;
      jset["id"] = nodeSet.getId();
      jset["label"] = nodeSet.getLabel();
      jset["partId"] = part->getId();
      jset["nodeIds"] = json::array();

      for (int nodeIndex = 0; nodeIndex < nodeSet.getItemCount(); ++nodeIndex) {
        const Node* node = nodeSet.getItem(nodeIndex);
        if (node != nullptr) {
          jset["nodeIds"].push_back(node->getId());
        }
      }

      m_json["NodeSets"].push_back(jset);
      totalNodeSets++;
    }
  }

  if (totalNodeSets > 0) {
    std::cout << "Done writing " << totalNodeSets << " node sets." << std::endl;
  }


  if (m_model.getMaterialCount()>0){
    
    if (m_model.getMaterial(0)!= nullptr){
    
    m_json["Materials"]["density0"]     = m_model.getMaterial(0)->getDensityConstant();  
    m_json["Materials"]["youngsModulus"]= m_model.getMaterial(0)->Elastic().E();  
    m_json["Materials"]["poissonsRatio"]= m_model.getMaterial(0)->Elastic().nu();  
    
    cout << "Checking plastic "<<endl;
    if (!m_model.getMaterial(0)->isPlastic()) {
      cout << "Elastic"<<endl;
      m_json["Materials"]["type"] = "Elastic";     
    } else {
      std::vector<double> plasticConst;
      if (m_model.getMaterial(0)->m_plastic){
        switch ( m_model.getMaterial(0)->m_plastic->Material_model ) {
                  
          case _GMT_: 
            cout << "GMT"<<endl;
            m_json["Materials"]["type"]= "GMT";  
            
            break;
          case HOLLOMON:
            cout << "Hollomon"<<endl;
            m_json["Materials"]["type"]= "Hollomon";  

                      
            break;
          default:
            break;
        }//Switch material model
        plasticConst = m_model.getMaterial(0)->m_plastic->getPlasticConstants(); //K,n
      } else {
          cout << "ERROR: No plastic present."<<endl;
      }
      if (plasticConst.size()>0)
        m_json["Materials"]["const"] = plasticConst;  // esto crea un array JSON automáticamente
      else
        cout << "ERROR: No plastic constants present."<<endl;
    }//PLASTIC
    
    }//if not nullptr
    cout << "Done."<<endl;
  }// MATERIALS

  if (m_model.getStepCount() > 0) {
    m_json["Steps"] = json::array();
    for (int i = 0; i < m_model.getStepCount(); ++i) {
      Step *step = m_model.getStep(i);
      if (!step)
        continue;

      json jstep;
      jstep["id"] = step->getId();
      jstep["name"] = step->getName();
      jstep["type"] = step->isImplicit() ? "Implicit" : "Explicit";
      jstep["nproc"] = step->m_nproc;
      jstep["cflFactor"] = step->m_cflFactor;
      jstep["autoTS"] = {step->m_autoTS[0], step->m_autoTS[1], step->m_autoTS[2]};
      jstep["kernelGradCorr"] = step->m_kernelGradCorr;
      jstep["simTime"] = step->m_simTime;
      jstep["artifViscAlpha"] = step->m_artifViscAlpha;
      jstep["artifViscBeta"] = step->m_artifViscBeta;
      jstep["outTime"] = step->m_outTime;
      jstep["fixedTS"] = step->m_fixedTS;
      jstep["axiSymmVol"] = step->m_axiSymmVol;
      jstep["elemLengthFraction"] = step->m_elemLengthFraction;
      jstep["meshing"]["debug"] = step->m_meshingDebug;
      jstep["meshing"]["maxElemAngle"] = step->m_maxElemAngle;
      jstep["meshing"]["minElemAngle"] = step->m_minElemAngle;
      jstep["implicit"]["type"] = step->m_implicitType;
      jstep["implicit"]["velTol"] = step->m_velTol;
      jstep["implicit"]["pressTol"] = step->m_pressTol;
      jstep["implicit"]["forceTol"] = step->m_forceTol;
      jstep["implicit"]["divTol"] = step->m_divTol;
      jstep["implicit"]["omegaV"] = step->m_omegaV;
      jstep["implicit"]["omegaP"] = step->m_omegaP;
      jstep["implicit"]["maxIter"] = step->m_maxIter;
      jstep["implicit"]["timeStepGrowthFactor"] = step->m_timeStepGrowthFactor;
      m_json["Steps"].push_back(jstep);
    }
  }

  if (m_model.getBCCount() > 0) {
      std::cout << "Writing Boundary Conditions..." << std::endl;

      m_json["BoundaryConditions"] = json::array();

      for (int i = 0; i < m_model.getBCCount(); ++i) {
          BoundaryCondition* bc = m_model.getBC(i);
          json jbc;

          // Tipo
          //std::string typeStr = (bc->getType() == VelocityBC) ? "VelocityBC" : "DisplacementBC";
          std::string typeStr;

          switch (bc->getType()) {
              case VelocityBC:       typeStr = "VelocityBC"; break;
              case DisplacementBC:   typeStr = "DisplacementBC"; break;
              case SymmetryBC:       typeStr = "SymmetryBC"; break;
              default:               typeStr = "Unknown"; break;
          }

          if (bc->getType() == SymmetryBC) {
              double3 n = bc->getNormal();
              jbc["normal"] = { n.x, n.y, n.z };
          }
          
          jbc["type"] = typeStr;

          // ApplyTo
          std::string applyToStr = (bc->getApplyTo() == ApplyToPart) ? "Part" : "NodeSet";
          jbc["applyTo"] = applyToStr;

          // ID objetivo
          jbc["targetId"] = bc->getTargetId();

          // Valor (velocidad o desplazamiento)
          double3 v = bc->getValue();
          jbc["value"] = {v.x, v.y, v.z};

          // Agregar al array
          m_json["BoundaryConditions"].push_back(jbc);
      }

      std::cout << "Done writing " << m_model.getBCCount() << " BCs." << std::endl;
  }  
  
  
  
  o << std::setw(4) << m_json << std::endl;
  
  o.close();
}
