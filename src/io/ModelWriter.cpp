
#include "ModelWriter.h"
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

ModelWriter::ModelWriter(Model& model) : m_model(model) {}


//~ void to_json(json& j, const Vec3& v) {
    //~ j = json{v.x, v.y, v.z};
//~ }


void ModelWriter::writeToFile(std::string fname){
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
  
  //SHOULD BE AT SPH
  //m_json["Configuration"]["hFactor"] = 1.2;
  m_json["Configuration"]["SPH"]["hFactor"] = 1.2;
  
  if (m_model.getMaterialCount()>0){
    cout << "Writing materials .."<<endl;
    m_json["Materials"]["density0"]=m_model.getMaterial(0)->getDensityConstant();  
    cout << "Done."<<endl;
  }


  cout << "Loop thorough parts..."<<endl;
  int i = 0;
  for (std::vector<Part*>::iterator it = m_model.m_part.begin(); it != m_model.m_part.end(); ++it){
   Part* part = *it;

      json jpart;
      jpart["id"] = part->getId();
      //~ jpart["name"] = part->getName();
      //~ jpart["material"] = part->getMaterialName();

      //~ // Mesh
      //~ jpart["mesh"] = json::object();
      //~ jpart["mesh"]["nodes"] = json::array();
      //~ for (auto& node : part->getNodes()) {
          //~ jpart["mesh"]["nodes"].push_back({node.x, node.y, node.z});
      //~ }
      if (part->isMeshed()){
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
          cout << "Part has geom"<<endl;
          jpart["geometry"]["source"] = part->getGeom()->getName();
          jpart["geometry"]["origin"] = writeVector(part->getGeom()->getOrigin());
          //jpart["geometry"]["representation"] = "BRep";
          //~ jpart["geometry"]["bounding_box"] = {
              //~ {part->getBBoxMinX(), part->getBBoxMinY(), part->getBBoxMinZ()},
              //~ {part->getBBoxMaxX(), part->getBBoxMaxY(), part->getBBoxMaxZ()}
          //~ };
      } 
      if (part->isMeshed()) {
        std::string meshname = "part_" + std::to_string(part->getId()) + ".msh";
        jpart["mesh"]["source"] = meshname;
      }

      m_json["model"]["parts"].push_back(jpart);
      
      
      json bc;
      
      bc["zoneId"] = 0;
      bc["value"] = {part->getVel().x,part->getVel().y,part->getVel().z};
      
      m_json["BoundaryConditions"].push_back(bc);
    
  }

  o << std::setw(4) << m_json << std::endl;
  
  o.close();
}
