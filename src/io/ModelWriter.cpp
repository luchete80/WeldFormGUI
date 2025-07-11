
#include "ModelWriter.h"
#include "Model.h"
#include "Part.h"
#include <iostream>
#include <fstream>
#include <iomanip> //setw
#include "Material.h"

#include <nlohmann/json.hpp>


using json = nlohmann::json;

ModelWriter::ModelWriter(Model& model) : m_model(model) {}

void ModelWriter::writeToFile(){
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
  std::ofstream o("WOW.json");
  std::string filename = m_model.getName();
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
  cout << "Saving model"<<endl;
  
  m_json["Configuration"]["modelType"] = "SPH";
  m_json["Configuration"]["solver"]    = "WeldForm";
  
  //SHOULD BE AT SPH
  //m_json["Configuration"]["hFactor"] = 1.2;
  m_json["Configuration"]["SPH"]["hFactor"] = 1.2;
  
  if (m_model.getMaterialCount()>0){
    cout << "Writing materials .."<<endl;
    m_json["Materials"]["density0"]=m_model.getMaterial(0)->getDensityConstant();  
    cout << "Done."<<endl;
  }
  /*
  m_model = &model;
  cout << "Loop thorough parts..."<<endl;
  for (std::vector<Part*>::iterator it = model.m_part.begin(); it != model.m_part.end(); ++it){
    //if (it->getType() == )
    //m_json["Parts"][] 
    }
  */
  o << std::setw(4) << m_json << std::endl;
  
  o.close();
}
