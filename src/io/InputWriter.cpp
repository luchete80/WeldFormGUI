
#include "InputWriter.h"
#include "Model.h"
#include "Part.h"
#include <iostream>
#include <fstream>
#include <iomanip> //setw
#include "Material.h"
#include "Geom.h"

#include "BoundaryCondition.h"

#include <nlohmann/json.hpp>
#include "json_io.h"

using json = nlohmann::json;

InputWriter::InputWriter(Model*m) : m_model(m) {}

void InputWriter::writeToFile(std::string fname){
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
  //std::ofstream o(m_model->getName());
  std::ofstream o(fname);
  std::string filename = m_model->getName();
  if (!m_model->getHasName()) cout << "Not has name!"<<endl;
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
  
  m_json["Configuration"]["cflFactor"] = 0.3;
  m_json["Configuration"]["fixedTS"] = true;
  m_json["Configuration"]["solver"]    = "WeldForm";

  m_json["Configuration"]["simTime"] = 1.0;
  m_json["Configuration"]["outTime"] =  1.0e-4;
  
  
  nlohmann::json cont;
  cont["fricCoeffStatic"] =  0.3;
  cont["fricCoeffDynamic"] =  0.3; 
  cont["penaltyFactor"] = 0.8;
  m_json["Contact"] = nlohmann::json::array();    
  m_json["Contact"].push_back(cont);
  
    //~ "Nproc":4,
  //~ "cflFactor": 0.3,
  //~ "autoTS": [false,false,false],

  //~ "simTime": 1.0e-3,

  //~ "outTime": 1.0e-5,
  //~ "fixedTS":true,
  //~ "domType": "AxiSymm",
  //~ "solver": "Mech-LeapFrog"
  
  //SHOULD BE AT SPH
  //m_json["Configuration"]["hFactor"] = 1.2;
  //m_json["Configuration"]["SPH"]["hFactor"] = 1.2;
  
  cout << "Writing materials .."<<endl;

  if (m_model->getMaterialCount() > 0) {
      m_json["Materials"] = nlohmann::json::array();

      for (int i = 0; i < m_model->getMaterialCount(); ++i) {
          auto* mat = m_model->getMaterial(i);
          if (mat == nullptr) continue;

          nlohmann::json mat_json;
          mat_json["density0"]      = mat->getDensityConstant();
          mat_json["poissonsRatio"] = mat->Elastic().nu();
          mat_json["youngsModulus"] = mat->Elastic().E();

          // Tipo de material
          if (!mat->isPlastic()) {
              mat_json["type"] = "Elastic";
          } else {
              switch (mat->m_plastic->Material_model) {
                  case _GMT_:
                      mat_json["type"] = "GMT";
                      break;
                  case HOLLOMON:
                      mat_json["type"] = "Hollomon";
                      break;
                  default:
                      mat_json["type"] = "UnknownPlastic";
                      break;
              }

              std::vector<double> plasticConst = mat->m_plastic->getPlasticConstants();
              if (!plasticConst.empty())
                  mat_json["const"] = plasticConst;
              else
                  cout << "WARNING: No plastic constants for material " << i << endl;
          }

          // Agregar al array principal
          m_json["Materials"].push_back(mat_json);
      }

      cout << "Done." << endl;
  }
 
  bool is_elastic = false;
  cout << "Loop thorough parts..."<<endl;
  for (std::vector<Part*>::iterator it = m_model->m_part.begin(); it != m_model->m_part.end(); ++it){
   Part* part = *it;
   
   if (part->getType() == Elastic){
     if (is_elastic){
        cout << "ERROR: More than Elastic body not supported. "<<endl;
        return;
      }
      json block;
     block["type"] = "File";

      if (part->isMeshed()){
      std::string fname = "part_" + std::to_string(part->getId()) + ".k";
      part->getMesh()->exportToLSDYNA(fname);
      block["fileName"] = fname;
      }
      else {
        
        cout << "ERROR: Part "<<part->getId()<< "is not meshed!"<<endl; 
        }


      m_json["DomainBlocks"].push_back(block);
     is_elastic = true;
  } else { ////RIGID

 
      json rigidBody;
      rigidBody["type"] = "File";
      rigidBody["zoneId"] = part->getId();
      rigidBody["start"] = {-0.02, -0.02, 0.03};
      rigidBody["dim"] = {0.04, 0.04, 0.0};
      rigidBody["translation"] = {1.0, 0.0, 0.0};
      rigidBody["scale"] = {1, 1, 1};


      if (part->isMeshed()){
        std::string fname = "part_" + std::to_string(part->getId()) + ".nas";
        part->getMesh()->exportToNASTRAN(fname);
        rigidBody["fileName"] = fname;
      }

      // Agregarlo al subbloque "RigidBodies"
      m_json["RigidBodies"].push_back(rigidBody);


      json bc;
      
      bc["zoneId"] = part->getId();
      bc["value"] = {part->getVel().x,part->getVel().y,part->getVel().z};
      
      m_json["BoundaryConditions"].push_back(bc);
      
  }

      json jpart;
      jpart["id"] = part->getId();
      
      //// if part is deformable
      
      //m_json["DomainBlocks"]["type"] = "File";


    

    }//Part 

    if (m_model->getBCCount() > 0) {
        std::cout << "Writing Boundary Conditions..." << std::endl;

        m_json["BoundaryConditions"] = json::array();

        for (int i = 0; i < m_model->getBCCount(); ++i) {
            BoundaryCondition* bc = m_model->getBC(i);
            if (!bc) continue;

            json jbc;

            // Tipo: Velocity o Displacement
            std::string typeStr = (bc->getType() == VelocityBC) ? "Velocity" : "DisplacementBC";
            //jbc["type"] = typeStr;
            
            jbc["valueType"] = 0; //Value, not amplitude

            // Aplicar a Part o Nodes
            std::string applyToStr = (bc->getApplyTo() == ApplyToPart) ? "Part" : "Nodes";
            jbc["applyTo"] = applyToStr;

            // ID del objetivo (parte o conjunto de nodos)
            jbc["zoneId"] = bc->getTargetId();

            // Valor (vector 3D)
            double3 v = bc->getVelocity();
            jbc["value"] = {v.x, v.y, v.z};

            // Agregar al array principal
            m_json["BoundaryConditions"].push_back(jbc);

            std::cout << "  → BC[" << i << "]: "
                      << typeStr << " | ApplyTo=" << applyToStr
                      << " | TargetID=" << bc->getTargetId()
                      << " | Value=(" << v.x << ", " << v.y << ", " << v.z << ")" << std::endl;
        }

        std::cout << "Done writing " << m_model->getBCCount() << " BCs." << std::endl;
    }
    
    if (!is_elastic)
      cout << "ERROR: Not deformable parts"<<endl;

  o << std::setw(4) << m_json << std::endl;
  
  o.close();
}
