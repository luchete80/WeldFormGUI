#include "Model.h"
#include "../../libs/LSDynaReader/src/lsdynaReader.h"
#include <iostream>
#include "Node.h"
#include "Mesh.h"
#include "Part.h"


#include "gmsh.h"
#include "ModelWriter.h"
#include "BoundaryCondition.h"
#include "InitialCondition.h"


//#include <gmsh.h>
//#include "GModel.h"
//#include "Geom.h"

//#include "drawContext.h"

using namespace std;
using namespace LS_Dyna;

//Mesh* Model::getPartMesh(const int &i){ if (have_meshes) return m_part[i]->getMesh();}
  
Model::Model(string name){
  part_count = 0;
  have_meshes = false;
  cout << "Creating Model ..."<<endl;
  cout << "Reading "<<name<<endl;
  string ext = name.substr(name.find_last_of(".")+1, name.length() - 1);
  cout << "extension: "<< ext<<endl;
  if (ext == "k"){
    lsdynaReader reader(name.c_str());
    cout << "creating "<<reader.m_node.size()<< " nodes."<<endl;
    for (int n=0;n<reader.m_node.size();n++){
      m_node.push_back(new Node(reader.m_node[n].m_x[0],reader.m_node[n].m_x[1],reader.m_node[n].m_x[2], 
                                reader.m_node[n].m_id));
      //Node ID is needed for element indices
    }
    /// IN THIS CASE, ELEMENT AND NODES ARE CREATED INSIDE MODEL
    for (int e = 0;e<reader.m_elem_count;e++){
      std::vector<Node*> enod;
      //cout << "node size"<<reader.m_elem[e].node.size()<<endl;
      for (int ne=0;ne<reader.m_elem[e].node.size();ne++){
        enod.push_back(m_node[reader.getNodePos(reader.m_elem[e].node[ne])]); //THIS CALLS A MAP
        // for (int t=0;t<3;t++)
        // msh->getElem(ne)->getNodeId(t);
        //reader.getNodePos(ne);
        //cout << reader.getNodePos(reader.m_elem[e].node[ne]) << " ";
      }
      //cout << endl;
      m_elem.push_back(new Element(enod)); //This vector is created in new element space

    }
    Mesh *msh = new Mesh;
    msh->assignValues(m_node, m_elem);
    m_part.push_back(new Part(msh));
    have_meshes = true;
  } else if (ext == "json"){
    
    
  } else if (ext == "step"){
      //test 
      bool errorIfMissing;


        // Load a STEP file (using `importShapes' instead of `merge' allows to
      // directly retrieve the tags of the highest dimensional imported entities):
      std::vector<std::pair<int, int> > v;
     //try {
        cout << "Loading file "<<name<<endl;
      int argc;
      char **argv;
      //gmsh::initialize(argc, argv);
      //gmsh::model::add("t20");
     /// gmsh::model::occ::importShapes(name, v);
     
     
     
      //GModel *m = GModel::current();
      //m= gmsh::model::geo::list[0];
      // = GModel::list[0];
      cout << "v size "<<v.size()<<endl;
      //gmsh_::model::drawContext::drawGeom();
    // Get the bounding box of the volume:
      double xmin, ymin, zmin, xmax, ymax, zmax;
//      gmsh::model::occ::getBoundingBox(v[0].first, v[0].second, xmin, ymin, zmin,
//                                       xmax, ymax, zmax);
//    double dx = (xmax - xmin);
//    double dy = (ymax - ymin);
//    double dz = (zmax - zmin);
///    cout << "dx dy dz "<<dx <<", "<<dy <<", "<<dz<<endl;

    //} catch(...) {
      //  gmsh::logger::write("Could not load STEP file: bye!");
      //  gmsh::finalize();
        //return 0;
      //}
  }
}

void Model::addPart(Part *part){
  cout << "Adding part"<<endl;
  int id = part->getId();
  cout << "ID GET"<<endl;
  std::vector<Entity*> nlist(m_part.begin(),m_part.end());
  cout << "Max ID "<<getMaxId(nlist);
  if (id>-1){
    
  }
  else {
    int id = part->getId();
    cout << "part id "<<id <<endl;
    if (m_part.size()>0){
      //cout << m_part[0]->getId()<<endl;
      //cout << "max id "<<endl;
      //cout << "Max ID "<<getMaxId(nlist);
      //part->setId(getMaxId(nlist));
    }
    
    //cout << "SET ID "<<endl;
    
    //TEST
    //processObjects<Entity>(m_part);
  }
  m_part.push_back(part);
  if (part->isMeshed()){
    for (int n=0;n<part->getMesh()->getNodeCount();n++){
      //Check if ID is not repeated
      m_node.push_back(part->getMesh()->getNode(n));
      
    }
  }  
}

void Model::addPart(Geom *geom){
  m_part.push_back(new Part(geom));
  m_part[m_part.size()-1]->setId(m_part.size()-1);
}

void Model::addGeom(Geom *geom){
  m_geom.push_back(geom);
}

void Model::addBoundaryCondition(BoundaryCondition *bc)
{
  m_bc.push_back(bc);
}

