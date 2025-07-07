#include "App.h"
#include <iostream>
#include "GraphicMesh.h"
#include "Part.h"
#include "Mesh.h"

//#include "VtkViewer.h"

using namespace std; 

App * App::_pcSingleton = nullptr;

/*static */ App &getApp(){
    //cout << "app "<<App::_pcSingleton<<endl;
    return *App::_pcSingleton;
}

void App::initApp(){
  cout << "Initializing App"<<endl;
    if (App::_pcSingleton == nullptr){
      App::_pcSingleton = new App();
      cout << "App address "<<App::_pcSingleton<<endl;
  }
}

bool & App::isUpdateNeeded(){
  
  return _updateNeeded;
  }


void App::setActiveModel(Model *m){
    _activeModel = m;
}

Model & App::getActiveModel(){
  if (_activeModel==nullptr)
    cout << "ERROR: No Active Model"<<endl;
  else 
    cout << "Model address"<<_activeModel<<endl;
  return *_activeModel;

}

void App::checkUpdate(){
  if (_updateNeeded){
      updateMeshes();
  }
}

////TODO: CHANGE TO CHECK FOR MESH->getPart()
////
void App::updateMeshes(){
  
  std::vector <bool> is_part;
  is_part.resize(m_graphicmeshes.size()); //to delete graphic mesh is part is not present
   
  if (_activeModel!= nullptr){
  cout << "searching on "<<_activeModel->getPartCount()<<" parts"<<endl;
  
  for (int p=0;p<_activeModel->getPartCount();p++){
    cout << "Checking app new parts "<<endl;
    bool not_found = true;
      for (int gm=0;gm<m_graphicmeshes.size();gm++){
        if (m_graphicmeshes[gm]->getMesh() == _activeModel->getPart(p)->getMesh()){//is related to the part mesh
            not_found = false;
            is_part[gm] = true;
          }
        
        }
      if (not_found){
        if (_activeModel->getPart(p)->isMeshed()){
          
        cout << "Creating Graphic Mesh for part "<<p<<endl;
        if (_activeModel->getPart(p)->getMesh()!=nullptr){
          if (_activeModel->getPart(p)->getMesh()->getType()==SPH){
            cout << "Creating SPH graphic mesh-----"<<endl;
            m_graphicmeshes.push_back( new GraphicSPHMesh(_activeModel->getPart(p)->getMesh())); ///THIS READS FROM GLOBAL GMSH MODEL
            m_graphicmeshes[m_graphicmeshes.size()-1]->createVTKPolyData(*_activeModel->getPart(p)->getMesh());
            //ACTOR IS NOT ASSIGNED (THIS IS DONE IN ORDER TO NOT ADD VTK CODE HERE)
            //viewer->addActor(graphic_mesh->getActor());  
          }
          else
          m_graphicmeshes.push_back(new GraphicMesh(_activeModel->getPart(p)->getMesh()));
          cout << "Created FEM mesh"<<endl; 
        }else 
          cout << "ERROR: Part mesh is null pointer"<<endl;
      
        cout << "Graphic mesh count is "<<m_graphicmeshes.size()<<endl;
      }
    }//is meshed
  } //part loop
    //NOT WORKING
  
  //Second loop fo deleted parts
    cout << "Looking for "<<m_graphicmeshes.size()<<" meshes and "<<_activeModel->getPartCount()<< " parts "<<endl;
    for (int gm=0;gm<m_graphicmeshes.size();gm++){
      bool del_part = true;
      for (int p=0;p<_activeModel->getPartCount();p++){
        cout << "sm mesh ptr "<<m_graphicmeshes[gm]->getMesh()<<", "<<" part msh pointer "<<_activeModel->getPart(p)->getMesh()<<endl;
      if (m_graphicmeshes[gm]->getMesh() == _activeModel->getPart(p)->getMesh()){//is related to the part mesh
        cout <<"Found one part with corresponding mesh"<<endl;
        del_part = false;
        }
      }
      if (del_part){
        cout<<"Deleting graphic mesh for unexisting part"<<endl; 
        ///REMOVE ACTOR 
        m_graphicmeshes.erase(m_graphicmeshes.begin()+gm);
      }
    }
  
      
  _updateNeeded = false;   
  
} else {
  cout <<"ERROR: No active model"<<endl;
  }
}



/* -------------------------------------------------------------------------- */
/* 1) Register an existing Geom created elsewhere (Python, UI, etc.)          */
Geom* App::addOrphanGeometry(Geom* g)
{
    if (!g) return nullptr;

    // wrap raw ptr into shared_ptr so we own the memory
    m_orphangeoms.emplace_back(g);
    _updateNeeded = true;                // force a sync at next frame
    return g;
}

/* -------------------------------------------------------------------------- */
/* 2) Convenience: load geometry from file and auto‑create visual+actor       */
Geom* App::loadGeometry(const std::string& file)
{
    Geom* geom = new Geom(file);          // ctor loads the STEP/BREP
    addOrphanGeometry(geom);
    return geom;
}

/* -------------------------------------------------------------------------- */
/* 3) Sync orphan geometries with viewer                                      */
void App::updateGeoms(/*vtkViewer* viewer*/) {
    std::cout << "Updating orphan geometries: " << m_orphangeoms.size() << std::endl;

    for (Geom* g : m_orphangeoms) {
        // Check if already visualized
        if (geomToVisual.find(g) == geomToVisual.end()) {
            std::cout << "Creating visual for: " << g->m_name << std::endl;

            vtkOCCTGeom* visual = new vtkOCCTGeom();
            visual->SetGeometry(g);         // Use the shape from Geom
            visual->BuildVTKData();              // Build VTK actor

            geomToVisual[g] = visual;            // Store in the map

            //~ if (viewer) {
                //~ viewer->addActor(visual->actor); // Add actor to the viewer (optional here)
            //~ }
        }
    }
}
