#ifndef _APP_H_
#define _APP_H_

#include "Model.h"
#include <iostream>
#include <vector>
#include "geom/vtkOCCTGeom.h"
#include "geom/Geom.h"
#include <unordered_map>
//#define MY_DLL_API __declspec(dllexport)
#include "Part.h"



using namespace std;

class GraphicMesh;
class VtkViewer;

//class VtkViewer;
//SHOULD ALSO BEE OF INTEREST TO HAVE A REFERENCE TO THE RENDERER
///SINGLETON

class /*__declspec(dllexport)*/  App{

public:  
  friend App &getApp();
  Model &getActiveModel();

  void setActiveModel(Model *);
  bool& isUpdateNeeded();
  void Update(){_updateNeeded=true;}
  void checkUpdate();
  static void initApp();
  void updateMeshes(); 
  void updateGeoms(); 
  std::unordered_map<Geom*, vtkOCCTGeom*> & getGeomToVisual(){return geomToVisual;}
//protected:
  explicit App(){
    _activeModel  = nullptr;
    _updateNeeded = false;}
  ~App(){}
  int  getGraphicMeshCount(){return m_graphicmeshes.size();}
  GraphicMesh * getGraphicMesh(int &i){return m_graphicmeshes[i];}
  
  Geom* addOrphanGeometry(Geom* g);
  Geom* loadGeometry(const std::string& file);

    void registerGeometry(Geom* geo, vtkOCCTGeom* visual);
    
    // Sobrecarga para cuando solo tienes la geometría y quieres que App cree el visual
    vtkOCCTGeom* registerGeometry(Geom* geo); 

    void registerPartVisual(Part* part, vtkOCCTGeom* visual) {
        if (!part || !visual) return;
        
        partToVisual[part] = visual;
        _updateNeeded = true;
        //cout << "Registered visual for part: " << part << " (" << part->getName() << ")" << endl;
    }
    
    vtkOCCTGeom* getVisualForPart(Part* part) {
        auto it = partToVisual.find(part);
        return (it != partToVisual.end()) ? it->second : nullptr;
    }
    
    bool hasVisualForPart(Part* part) const {
        return partToVisual.find(part) != partToVisual.end();
    }
    
    // Función para limpiar visualizaciones de partes eliminadas
    void cleanupOrphanVisuals(Model* model) {
        std::vector<Part*> toRemove;
        for (const auto& pair : partToVisual) {
            bool partExists = false;
            for (int i = 0; i < model->getPartCount(); i++) {
                if (model->getPart(i) == pair.first) {
                    partExists = true;
                    break;
                }
            }
            if (!partExists) {
                toRemove.push_back(pair.first);
            }
        }
        
        for (Part* part : toRemove) {
            cout << "Cleaning up visual for deleted part: " << part << endl;
            partToVisual.erase(part);
        }
    }
    
private:
  static App *_pcSingleton;  
  Model *_activeModel;
  bool _updateNeeded;
  std::vector <GraphicMesh *> m_graphicmeshes;
  std::vector <vtkOCCTGeom*> m_geoms;
    //std::vector<std::shared_ptr<Geometry>> m_orphanGeometries;
  std::vector <Geom*> m_orphangeoms;
  //REDUNDANT
  std::unordered_map<Geom*, vtkOCCTGeom*> geomToVisual; //TO DELETE
  std::unordered_map<Part*, vtkOCCTGeom*> partToVisual; // ID de parte -> visual

}; 



/*static */ 
App &getApp();


#endif
