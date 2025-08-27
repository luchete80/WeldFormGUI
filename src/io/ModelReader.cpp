
#include "ModelReader.h"
#include "Model.h"
#include "Part.h"
#include <iostream>
#include <fstream>
#include <iomanip> //setw
#include "Material.h"
#include "Geom.h"

ModelReader::ModelReader(Model *model){
  
  m_model = model;
  }
ModelReader::ModelReader(const char *fname){

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
  //std::ofstream o("pretty.json");

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
/*
  cout << "Saving model"<<endl;
  
  m_json["Configuration"]["modelType"] = "SPH";
  m_json["Configuration"]["solver"]    = "WeldForm";
  
  //SHOULD BE AT SPH
  //m_json["Configuration"]["hFactor"] = 1.2;
  m_json["Configuration"]["SPH"]["hFactor"] = 1.2;
  
  if (model.getMaterialCount()>0){
    cout << "Writing materials .."<<endl;
    m_json["Materials"]["density0"]=model.getMaterial(0)->getDensityConstant();  
    cout << "Done."<<endl;
  }

  o << std::setw(4) << m_json << std::endl;
  
  o.close();
  */
}



bool ModelReader::readFromFile(const std::string& fname) {
    std::ifstream i(fname);
    if (!i.is_open()) {
        std::cerr << "Error opening file: " << fname << std::endl;
        return false;
    }

    json j;
    try {
        i >> j;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
        return false;
    }
    
    //m_model = new Model();

    // Configuración básica
    if (j.contains("Configuration")) {
        auto& conf = j["Configuration"];
        //~ if (conf.contains("modelType"))
            //~ //m_model.setType(conf["modelType"].get<std::string>());
        //~ if (conf.contains("solver"))
            //~ //m_model.setSolver(conf["solver"].get<std::string>());
        //~ if (conf.contains("SPH") && conf["SPH"].contains("hFactor"))
            //~ //m_model.setHFactor(conf["SPH"]["hFactor"].get<double>());
    }

    // Materiales
    if (j.contains("Materials")) {
        auto& mat = j["Materials"];
        if (mat.contains("density0")) {
            Material_* m = new Material_();
            //m->setDensity(mat["density0"].get<double>());
            m_model->addMaterial(m);
        }
    }

    //~ // Partes
    if (j.contains("model") && j["model"].contains("parts")) {
      int i=0;
    for (auto& jpart : j["model"]["parts"]) {
            cout << "Reading part "<<i<<endl;
            Part* part;
            //~ if (jpart.contains("id"))
                //~ part->setId(jpart["id"].get<int>());
            //~ if (jpart.contains("name"))
                //~ part->setName(jpart["name"].get<std::string>());

            //~ // Geometry
            if (jpart.contains("geometry")) {
                if (jpart["geometry"].contains("source")){
                    std::string name = jpart["geometry"]["source"].get<std::string>();
                    cout << "Reading surface "<<name<<endl;
                    Geom* geom = new Geom(name);
                 part = new Part(geom);
               }
                //part->setGeom(geom);
            }

            //~ // Mesh (nodos y elementos, si existe)
            //~ if (jpart.contains("mesh")) {
                //~ if (jpart["mesh"].contains("nodes")) {
                    //~ for (auto& n : jpart["mesh"]["nodes"]) {
                        //~ part->addNode({n[0].get<double>(), n[1].get<double>(), n[2].get<double>()});
                    //~ }
                //~ }
                //~ if (jpart["mesh"].contains("elements")) {
                    //~ for (auto& e : jpart["mesh"]["elements"]) {
                        //~ Element elem;
                        //~ elem.id = e["id"].get<int>();
                        //~ elem.type = e["type"].get<std::string>();
                        //~ elem.connectivity = e["connectivity"].get<std::vector<int>>();
                        //~ part->addElement(elem);
                    //~ }
                //~ }
            //~ }

            m_model->addPart(part);
            i++;
        }
    }//if contain part

    std::cout << "Model loaded from " << fname << std::endl;
    return true;
}

