
#include "InputWriter.h"
#include "Model.h"
#include "Part.h"
#include <iostream>
#include <fstream>
#include <iomanip> //setw
#include "Material.h"
#include "Geom.h"

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
  m_json["Configuration"]["SPH"]["hFactor"] = 1.2;
  
  
  
  if (m_model->getMaterialCount()>0){
    cout << "Writing materials .."<<endl;
    m_json["Materials"]["density0"]=m_model->getMaterial(0)->getDensityConstant();  
    cout << "Done."<<endl;
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
     m_json["DomainBlocks"]["type"] = "File";

      if (part->isMeshed()){
      std::string fname = "part_" + std::to_string(part->getId()) + ".k";
      part->getMesh()->exportToLSDYNA(fname);
      m_json["DomainBlocks"]["fileName"] = fname;
      }
      else {
        
        cout << "ERROR: Part "<<part->getId()<< "is not meshed!"<<endl; 
        }



     is_elastic = true;
  } else { ////RIGID

 
      json rigidBody;
      rigidBody["type"] = "File";
      rigidBody["zoneId"] = part->getId();
      rigidBody["start"] = {-0.02, -0.02, 0.03};
      rigidBody["dim"] = {0.04, 0.04, 0.0};
      rigidBody["translation"] = {1.0, 0.0, 0.0};
      rigidBody["scale"] = {1, 1, 1};

      // Agregarlo al subbloque "RigidBodies"
      m_json["RigidBodies"].push_back(rigidBody);

      if (part->isMeshed()){
        std::string fname = "part_" + std::to_string(part->getId()) + ".nas";
        part->getMesh()->exportToNASTRAN(fname);
        m_json["DomainBlocks"]["fileName"] = fname;
      }

  }

      json jpart;
      jpart["id"] = part->getId();
      
      //// if part is deformable
      
      //m_json["DomainBlocks"]["type"] = "File";


    }//Part 
    
    if (!is_elastic)
      cout << "ERROR: Not deformable parts"<<endl;

  o << std::setw(4) << m_json << std::endl;
  
  o.close();
}
