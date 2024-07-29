#include "input_writer.h"
#include <sstream>
#include <fstream>
#include "json.hpp"
#include "Model.h"
#include "Material.h"

using namespace std;

using json = nlohmann::json;

InputWriter::InputWriter (const string &str, const SPHModel &dom) {
  

  std::ofstream of(str, std::ios::out);
  
  json j;
  
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
  
  j["pi"] = 3.14;
  if (dom.getMaterialCount()>0)
    j["Materials"]["density0"]=dom.getMaterial(0)->getDensityConstant();
  
  of << j <<endl;
	of.close();
  
  
}