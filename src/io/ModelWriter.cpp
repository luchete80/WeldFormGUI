
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
#include "BoundaryCondition.h"

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
  
  if (m_model.m_thermal_coupling)
    m_json["Configuration"]["thermal"] = true; 
  
  //SHOULD BE AT SPH
  //m_json["Configuration"]["hFactor"] = 1.2;
  m_json["Configuration"]["SPH"]["hFactor"] = 1.2;
  
  if (m_model.getMaterialCount()>0){
    cout << "Writing materials .."<<endl;
    m_json["Materials"]["density0"]      = m_model.getMaterial(0)->getDensityConstant();  
    m_json["Materials"]["poissonsRatio"] = m_model.getMaterial(0)->Elastic().nu();  
    m_json["Materials"]["youngsModulus"] = m_model.getMaterial(0)->Elastic().E();  
    
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
          //If save as first time or save as
          if (m_model.getPrevName()!=m_model.getName()){
            std::string name = m_model.getName()+"_part_" + std::to_string(i) + ".step";
            part->getGeom()->setFileName(name);
            part->getGeom()->ExportSTEP();
          }

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
        cout << "Part "<<i << " is meshed."<<endl;
        std::string meshname = m_model.getName() + "_part_" + std::to_string(part->getId()) + ".msh";
        jpart["mesh"]["source"] = meshname;


        //CHANGE THIS BY EXPORTING TO LS-DYNA
        if (m_model.getPrevName()!=m_model.getName()){
         std::string kname = m_model.getName()+"_part_" + std::to_string(i) + ".k";
            cout << "Exporting to LSDyna..."<<endl;
            m_model.getPart(i)->getMesh()->exportToLSDYNA(kname);
        


            std::string old_name = m_model.getPrevName() + "_part_" + std::to_string(i) + ".msh";
            std::string new_name = m_model.getName() + "_part_" + std::to_string(i) + ".msh";

            if (std::rename(old_name.c_str(), new_name.c_str()) == 0) {
                std::cout << "Mesh file renamed: " << old_name << " -> " << new_name << std::endl;
            } else {
                std::perror(("Error renaming " + old_name).c_str());
            }
        
        }// IF NAME CHANGED
      } else {
          cout << "Part "<<i << " is not meshed."<<endl;
      }

      m_json["model"]["parts"].push_back(jpart);
      
      i++;
    
  }//PART


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
          std::string applyToStr = (bc->getApplyTo() == ApplyToPart) ? "Part" : "Nodes";
          jbc["applyTo"] = applyToStr;

          // ID objetivo
          jbc["targetId"] = bc->getTargetId();

          // Valor (velocidad o desplazamiento)
          double3 v = bc->getVelocity();
          jbc["value"] = {v.x, v.y, v.z};

          // Agregar al array
          m_json["BoundaryConditions"].push_back(jbc);
      }

      std::cout << "Done writing " << m_model.getBCCount() << " BCs." << std::endl;
  }  
  
  
  
  o << std::setw(4) << m_json << std::endl;
  
  o.close();
}
