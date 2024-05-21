#include "Model.h"
#include "../../libs/LSDynaReader/src/lsdynaReader.h"
#include <iostream>
#include "Node.h"
#include "Mesh.h"


using namespace std;
using namespace LS_Dyna;

Model::Model(string name){
  cout << "Reading "<<name<<endl;
  lsdynaReader reader(name.c_str());
  
  for (int n=0;n<reader.m_node.size();n++){
    m_node.push_back(new Node(reader.m_node[n].m_x[0],reader.m_node[n].m_x[1],reader.m_node[n].m_x[2]));
  }
  for (int e = 0;e<reader.m_elem_count;e++){
    std::vector<Node*> enod;
    for (int ne=0;ne<reader.m_elem[e].node.size();ne++){
      enod.push_back(m_node[reader.m_elem[e].node[ne]]);
      //reader.getNodePos(ne);
      
    }
    
  }
  Mesh *msh = new Mesh;

  
}