#include "App.h"
#include <iostream>
#include "GraphicMesh.h"
#include "Part.h"
#include "Mesh.h"
#include <algorithm>
#include <fstream>
#include <filesystem>

//#include "VtkViewer.h"

using namespace std; 

namespace {
void removeGraphicMeshInstance(std::vector<GraphicMesh*>& graphicMeshes,
                               std::vector<GraphicMesh*>::iterator& it,
                               std::vector<vtkSmartPointer<vtkProp>>& pendingActorRemovals) {
    GraphicMesh* gmesh = *it;
    if (gmesh != nullptr) {
        if (gmesh->getActor() != nullptr) {
            pendingActorRemovals.push_back(gmesh->getActor());
        }
        delete gmesh;
    }
    it = graphicMeshes.erase(it);
}
}

App * App::_pcSingleton = nullptr;

/*static */ App &getApp(){
    //cout << "app "<<App::_pcSingleton<<endl;
    return *App::_pcSingleton;
}

void App::initApp(){
  cout << "Initializing App"<<endl;
    if (App::_pcSingleton == nullptr){
      App::_pcSingleton = new App();
      App::_pcSingleton->loadRecentFiles();
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
  else {
    //cout << "Model address"<<_activeModel<<endl;
  }
  return *_activeModel;

}

void App::checkUpdate(){
  if (_updateNeeded){
      updateMeshes();
      updateGeoms(); //Orphan Geoms
  }
}

////TODO: CHANGE TO CHECK FOR MESH->getPart()
////
void App::updateMeshes(){

  if (_activeModel!= nullptr){
  cout << "searching on "<<_activeModel->getPartCount()<<" parts"<<endl;
  
  for (int p=0;p<_activeModel->getPartCount();p++){
    Part* part = _activeModel->getPart(p);
    if (!part || !part->isMeshed()) {
      continue;
    }
    cout << "Checking app new parts "<<endl;
    cout << "Graphics meshes size "<<m_graphicmeshes.size()<<endl;
    bool not_found = true;
      Mesh* partMesh = part->getMesh();
      if (partMesh == nullptr) {
        continue;
      }
      for (int gm=0;gm<m_graphicmeshes.size();gm++){
        if (m_graphicmeshes[gm]->getMesh() == partMesh){//is related to the part mesh
            not_found = false;
          }
        
        }
      if (not_found){
        if (part->isMeshed()){
          
        cout << "Creating Graphic Mesh for part "<<p<<endl;
        if (partMesh != nullptr){
          if (partMesh->getType()==SPH){
            cout << "Creating SPH graphic mesh-----"<<endl;
            m_graphicmeshes.push_back( new GraphicSPHMesh(partMesh)); ///THIS READS FROM GLOBAL GMSH MODEL
            m_graphicmeshes[m_graphicmeshes.size()-1]->createVTKPolyData(*partMesh);
            //ACTOR IS NOT ASSIGNED (THIS IS DONE IN ORDER TO NOT ADD VTK CODE HERE)
            //viewer->addActor(graphic_mesh->getActor());  
          }
          else
          m_graphicmeshes.push_back(new GraphicMesh(partMesh));
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
    auto gmIt = m_graphicmeshes.begin();
    while (gmIt != m_graphicmeshes.end()){
      bool del_part = true;
      GraphicMesh* graphicMesh = *gmIt;
      for (int p=0;p<_activeModel->getPartCount();p++){
        Part* part = _activeModel->getPart(p);
        if (!part || !part->isMeshed()) {
          continue;
        }
        Mesh* partMesh = part->getMesh();
        if (partMesh == nullptr) {
          continue;
        }
        cout << "sm mesh ptr "<<graphicMesh->getMesh()<<", "<<" part msh pointer "<<partMesh<<endl;
      if (graphicMesh->getMesh() == partMesh){//is related to the part mesh
        cout <<"Found one part with corresponding mesh"<<endl;
        del_part = false;
        break;
        }
      }
      if (del_part){
        cout<<"Deleting graphic mesh for unexisting part"<<endl; 
        removeGraphicMeshInstance(m_graphicmeshes, gmIt, m_pendingActorRemovals);
      } else {
        ++gmIt;
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
            //std::cout << "Creating visual for: " << g->m_name << std::endl;

            vtkOCCTGeom* visual = new vtkOCCTGeom();
            visual->SetGeometry(g);         // Use the shape from Geom
            visual->BuildVTKData();              // Build VTK actor
            visual->isRendered = false;
            
            geomToVisual[g] = visual;            // Store in the map

            //~ if (viewer) {
                //~ viewer->addActor(visual->actor); // Add actor to the viewer (optional here)
            //~ }
        }
    }
}


void App::registerGeometry(Geom* geo, vtkOCCTGeom* visual) {
    if (!geo || !visual) return;
    
    visual->SetGeometry(geo);
    if (visual->actor == nullptr) {
        cout << "[registerGeometry] building new visual for geom " << geo << endl;
        visual->BuildVTKData();
        visual->isRendered = false;
    } else {
        cout << "[registerGeometry] reusing existing visual for geom " << geo
             << " actor=" << visual->actor.GetPointer() << endl;
    }
    
    geomToVisual[geo] = visual;
    if (std::find(m_orphangeoms.begin(), m_orphangeoms.end(), geo) == m_orphangeoms.end()) {
        m_orphangeoms.emplace_back(geo);
    }
    _updateNeeded = true;
    
    //std::cout << "Registered geometry: " << geo->m_name << std::endl;
}

void App::removeGraphicMeshForPart(Part* part) {
    if (!part) return;

    // Buscamos los GraphicMesh asociados a la parte
    auto it = m_graphicmeshes.begin();
    while (it != m_graphicmeshes.end()) {
        GraphicMesh* gmesh = *it;
        if (gmesh && gmesh->getMesh() == part->getMesh()) {
            removeGraphicMeshInstance(m_graphicmeshes, it, m_pendingActorRemovals);

            std::cout << "Removed GraphicMesh and actor for part Id: "
                      << part->getId() << std::endl;
        } else {
            ++it;
        }
    }

    _updateNeeded = true;
}

void App::clearVisualsForModel(Model* model) {
    if (!model) return;

    cout << "[clearVisualsForModel] begin for model " << model
         << " parts=" << model->getPartCount()
         << " graphicMeshes=" << m_graphicmeshes.size()
         << " geomToVisual=" << geomToVisual.size()
         << " partToVisual=" << partToVisual.size()
         << " orphanGeoms=" << m_orphangeoms.size() << endl;

    auto gmIt = m_graphicmeshes.begin();
    while (gmIt != m_graphicmeshes.end()) {
        removeGraphicMeshInstance(m_graphicmeshes, gmIt, m_pendingActorRemovals);
    }

    for (int i = 0; i < model->getPartCount(); ++i) {
        Part* part = model->getPart(i);
        if (!part) continue;

        cout << "[clearVisualsForModel] part[" << i << "]=" << part
             << " geom=" << part->getGeom()
             << " mesh=" << part->getMesh() << endl;

        Geom* geom = part->getGeom();
        if (geom != nullptr) {
            auto geomVisualIt = geomToVisual.find(geom);
            if (geomVisualIt != geomToVisual.end()) {
                vtkOCCTGeom* geomVisual = geomVisualIt->second;
                if (geomVisual && geomVisual->actor) {
                    cout << "[clearVisualsForModel] queue geom visual actor "
                         << geomVisual->actor.GetPointer()
                         << " class=" << geomVisual->actor->GetClassName()
                         << " for geom " << geom << endl;
                    m_pendingActorRemovals.push_back(geomVisual->actor);
                }
                if (geomVisual != nullptr) {
                    bool alsoOwnedByPart = false;
                    auto visualIt = partToVisual.find(part);
                    if (visualIt != partToVisual.end() && visualIt->second == geomVisual)
                        alsoOwnedByPart = true;
                    cout << "[clearVisualsForModel] geom visual " << geomVisual
                         << " alsoOwnedByPart=" << alsoOwnedByPart << endl;
                    if (!alsoOwnedByPart)
                        delete geomVisual;
                }
                geomToVisual.erase(geomVisualIt);
            } else {
                cout << "[clearVisualsForModel] no geomToVisual entry for geom " << geom << endl;
            }

            m_orphangeoms.erase(
                std::remove(m_orphangeoms.begin(), m_orphangeoms.end(), geom),
                m_orphangeoms.end());
        }

        auto visualIt = partToVisual.find(part);
        if (visualIt != partToVisual.end()) {
            vtkOCCTGeom* visual = visualIt->second;
            if (visual && visual->actor) {
                cout << "[clearVisualsForModel] queue part visual actor "
                     << visual->actor.GetPointer()
                     << " class=" << visual->actor->GetClassName()
                     << " for part " << part << endl;
                m_pendingActorRemovals.push_back(visual->actor);
            }
            cout << "[clearVisualsForModel] deleting part visual " << visual
                 << " for part " << part << endl;
            delete visual;
            partToVisual.erase(visualIt);
        } else {
            cout << "[clearVisualsForModel] no partToVisual entry for part " << part << endl;
        }
    }

    cout << "[clearVisualsForModel] end pendingRemovals=" << m_pendingActorRemovals.size()
         << " geomToVisual=" << geomToVisual.size()
         << " partToVisual=" << partToVisual.size()
         << " orphanGeoms=" << m_orphangeoms.size() << endl;

    _updateNeeded = true;
}


GraphicMesh* App::getGraphicMeshFromPart(Part* part) {
    if (!part) return nullptr;

    // Buscamos los GraphicMesh asociados a la parte
    auto it = m_graphicmeshes.begin();
    while (it != m_graphicmeshes.end()) {
        GraphicMesh* gmesh = *it;
        if (gmesh && gmesh->getMesh() == part->getMesh()) {
            return gmesh;
        }
        ++it;
    }
    return nullptr;
}

// Sobrecarga para cuando solo tienes la geometría y quieres que App cree el visual
vtkOCCTGeom* App::registerGeometry(Geom* geo) {
    if (!geo) return nullptr;
    
    vtkOCCTGeom* visual = new vtkOCCTGeom();
    registerGeometry(geo, visual);
    return visual;
}

std::filesystem::path App::recentFilesStoragePath() const
{
    return std::filesystem::current_path() / ".recent_wfmodels";
}

void App::loadRecentFiles()
{
    m_recentFiles.clear();

    std::ifstream input(recentFilesStoragePath());
    if (!input.is_open()) {
        return;
    }

    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty()) {
            m_recentFiles.push_back(line);
        }
    }
}

void App::saveRecentFiles() const
{
    std::ofstream output(recentFilesStoragePath());
    if (!output.is_open()) {
        std::cerr << "Failed to write recent files list." << std::endl;
        return;
    }

    for (const std::string& path : m_recentFiles) {
        output << path << '\n';
    }
}

void App::addRecentFile(const std::string& path)
{
    if (path.empty()) {
        return;
    }

    std::error_code ec;
    const std::string normalized =
        std::filesystem::absolute(path, ec).lexically_normal().string();
    const std::string& entry = ec ? path : normalized;

    m_recentFiles.erase(
        std::remove(m_recentFiles.begin(), m_recentFiles.end(), entry),
        m_recentFiles.end());
    m_recentFiles.insert(m_recentFiles.begin(), entry);

    static const std::size_t kMaxRecentFiles = 10;
    if (m_recentFiles.size() > kMaxRecentFiles) {
        m_recentFiles.resize(kMaxRecentFiles);
    }

    saveRecentFiles();
}
