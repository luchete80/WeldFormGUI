#include "GraphicMesh.h"

#include <vtkActor.h>
#include <vtkCellArray.h>
#include <vtkFloatArray.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <gmsh.h>
#include <vtkLine.h>
#include <vtkTriangle.h>
#include <vtkQuad.h>
#include <vtkProperty.h>
#include <vtkTetra.h>

#include <array>
#include <array>

#include "VtkViewer.h"
// ADD VTK MECH
// FROM https://examples.vtk.org/site/Cxx/GeometricObjects/Cube/

//SPH Meshes are based on glyphs
//https://examples.vtk.org/site/Cxx/Filtering/Glyph3D/

#include <vtkSphereSource.h> 

#include <iostream>
#include <fstream>
#include "Mesh.h"
#include "Node.h"
#include "Element.h"

void GraphicMesh::setPoints(Mesh &mesh){

  std::vector <std::array<float,3>> pts; 
  
  points = vtkSmartPointer<vtkPoints>::New();
  cout << "Node count "<<mesh.getNodeCount()<<endl;
  for (int n=0;n<mesh.getNodeCount();n++){
    //cout << "Node "<<n<<endl;
          std::array <float,3> coords;
          for (int d=0;d<3;d++) coords[d] = mesh.m_node[n]->getPos()[d];
          // IF REAL POSITIONS
          pts.push_back(coords);
  }
  cout << "Inserting nodes"<<endl;
  for (auto i = 0ul; i < pts.size(); ++i)
  {
    //cout << "Node"<<i<<endl;
    points->InsertPoint(i, pts[i].data());
    //scalars->InsertTuple1(i, i);
  }
  cout << "Done"<<endl;  
}

int GraphicMesh::createVTKPolyData() {
  std::vector <std::array<float,3>> pts; 

  //std::vector <std::array<int,3>> elnodes; 
  std::vector <std::vector <int> > elnodes; 
  std::map< int,int > nodetagpos;
  
    // Limpiar estructuras existentes
    pts.clear();
    elnodes.clear();
    nodetagpos.clear();

    // Obtener información del modelo
    std::string name;
    gmsh::model::getCurrent(name);
    std::cout << "Model " << name << " (" << gmsh::model::getDimension() << "D)\n";

    // Obtener todas las entidades del modelo
    std::vector<std::pair<int, int>> entities;
    gmsh::model::getEntities(entities);

    // Mapa para relacionar tags de Gmsh con índices de VTK
    std::map<std::size_t, vtkIdType> gmshToVtkIndex;
    vtkNew<vtkPoints> points;
    vtkNew<vtkCellArray> cells;
    vtkNew<vtkFloatArray> scalars;

    // Primera pasada: recoger todos los puntos y crear mapeo
    int nodeCount = 0;
    for (auto &e : entities) {
        int dim = e.first, tag = e.second;
        
        std::vector<std::size_t> nodeTags;
        std::vector<double> nodeCoords, nodeParams;
        gmsh::model::mesh::getNodes(nodeTags, nodeCoords, nodeParams, dim, tag);
        
        for (int i = 0; i < nodeTags.size(); i++) {
            if (gmshToVtkIndex.find(nodeTags[i]) == gmshToVtkIndex.end()) {
                std::array<float, 3> coord = {
                    static_cast<float>(nodeCoords[3 * i]),
                    static_cast<float>(nodeCoords[3 * i + 1]),
                    static_cast<float>(nodeCoords[3 * i + 2])
                };
                
                pts.push_back(coord);
                vtkIdType pointId = points->InsertNextPoint(coord[0], coord[1], coord[2]);
                gmshToVtkIndex[nodeTags[i]] = pointId;
                scalars->InsertTuple1(pointId, pointId);
                nodeCount++;
            }
        }
    }

    std::cout << "Total nodes: " << nodeCount << std::endl;

    // Segunda pasada: procesar elementos
    for (auto &e : entities) {
        int dim = e.first, tag = e.second;
        
        std::vector<int> elemTypes;
        std::vector<std::vector<std::size_t>> elemTags, elemNodeTags;
        gmsh::model::mesh::getElements(elemTypes, elemTags, elemNodeTags, dim, tag);

        for (int i = 0; i < elemTypes.size(); i++) {
            int elemType = elemTypes[i];
            std::string elemName;
            int elemDim, order, numNodes, numPrimaryNodes;
            std::vector<double> param;
            
            gmsh::model::mesh::getElementProperties(elemType, elemName, elemDim, order, 
                                                   numNodes, param, numPrimaryNodes);
            
            std::cout << "Processing element type: " << elemName << " with " << numNodes << " nodes" << std::endl;

            for (int j = 0; j < elemTags[i].size(); j++) {
                vtkSmartPointer<vtkCell> cell;
                std::vector<int> connectivity;
                
                // Crear el tipo de celda apropiado
                if (elemDim == 1 && numNodes == 2) {
                    cell = vtkSmartPointer<vtkLine>::New();
                } else if (elemDim == 2 && numNodes == 3) {
                    cell = vtkSmartPointer<vtkTriangle>::New();
                } else if (elemDim == 2 && numNodes == 4) {
                    cell = vtkSmartPointer<vtkQuad>::New();
                } else {
                    std::cout << "Unsupported element type: " << elemName << std::endl;
                    continue;
                }

                // Establecer la conectividad de la celda
                for (int k = 0; k < numNodes; k++) {
                    std::size_t nodeTag = elemNodeTags[i][j * numNodes + k];
                    if (gmshToVtkIndex.find(nodeTag) != gmshToVtkIndex.end()) {
                        vtkIdType pointId = gmshToVtkIndex[nodeTag];
                        cell->GetPointIds()->SetId(k, pointId);
                        connectivity.push_back(pointId);
                    } else {
                        std::cerr << "Error: Node tag " << nodeTag << " not found in mapping!" << std::endl;
                    }
                }
                
                // Añadir la celda y guardar conectividad
                cells->InsertNextCell(cell);
                elnodes.push_back(connectivity);
            }
        }
    }

    std::cout << "Total elements: " << elnodes.size() << std::endl;

    // Crear el polydata
    mesh_pdata = vtkSmartPointer<vtkPolyData>::New();
    mesh_pdata->SetPoints(points);
    
    // Configurar según la dimensión de los elementos
    bool has2DElements = false;
    bool has1DElements = false;
    
    for (auto &e : entities) {
        if (e.first == 2) has2DElements = true;
        if (e.first == 1) has1DElements = true;
    }
    
    if (has2DElements) {
        mesh_pdata->SetPolys(cells);
    }
    if (has1DElements) {
        mesh_pdata->SetLines(cells);
    }
    
    mesh_pdata->GetPointData()->SetScalars(scalars);

    // Configurar mapper y actor
    mesh_Mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mesh_Mapper->SetInputData(mesh_pdata);
    mesh_Mapper->SetScalarRange(mesh_pdata->GetScalarRange());
    
    mesh_actor = vtkSmartPointer<vtkActor>::New();
    mesh_actor->SetMapper(mesh_Mapper);
    
    vtkNew<vtkNamedColors> colors;
    mesh_actor->GetProperty()->SetColor(colors->GetColor3d("Silver").GetData());
    mesh_actor->GetProperty()->EdgeVisibilityOn();
    mesh_actor->GetProperty()->SetEdgeColor(0.0, 0.0, 0.0);
    
    // Solo mostrar wireframe si hay elementos 2D
    if (has2DElements) {
        mesh_actor->GetProperty()->SetRepresentationToWireframe();
    }
    
    mesh_actor->Modified();

    std::cout << "VTK PolyData created successfully!" << std::endl;
    return EXIT_SUCCESS;
}

//~ int GraphicMesh::createVTKPolyData(Mesh &mesh)
//~ {
  //~ m_needs_polydata = false;
  //~ m_needs_actor = true; //needs actor to be shown
  //~ mesh_actor = nullptr;
  //~ mesh_pdata = nullptr;
  
  //~ mesh_pdata = vtkSmartPointer<vtkPolyData>::New();
  
  //~ std::vector <std::array<float,3>> pts; 
  
  //~ points = vtkSmartPointer<vtkPoints>::New();
  //~ cout << "Node count "<<mesh.getNodeCount()<<endl;
  //~ for (int n=0;n<mesh.getNodeCount();n++){
    //~ //cout << "Node "<<n<<endl;
          //~ std::array <float,3> coords;
          //~ for (int d=0;d<3;d++) coords[d] = mesh.m_node[n]->getPos()[d];
          //~ // IF REAL POSITIONS
          //~ pts.push_back(coords);
    //~ for (int d=0; d<3; d++) {
        //~ coords[d] = mesh.m_node[n]->getPos()[d];
        //~ //cout << "Raw coord " << d << ": " << coords[d] << endl;
    //~ }

    //~ points->InsertPoint(n, coords.data());
    //~ // Verify what was actually inserted
    //~ double inserted[3];
    //~ points->GetPoint(n, inserted);
    //~ //cout << "Inserted coord: " 
    //~ //     << inserted[0] << ", " << inserted[1] << ", " << inserted[2] << endl;
    //~ }

  //~ // cout << "Inserting nodes"<<endl;
  //~ // for (auto i = 0ul; i < pts.size(); ++i)
  //~ // {
    //~ // //cout << "Node"<<i<<endl;
    //~ // points->InsertPoint(i, pts[i].data());
    //~ // //scalars->InsertTuple1(i, i);
  //~ // }
  //~ // cout << "Done"<<endl;

  //~ if (mesh.getType()==FEM){
  //~ cout << "Mesh Type is FEM"<<endl;
  //~ polys  = vtkSmartPointer<vtkCellArray>::New();

  //~ for (int e=0;e<mesh.getElemCount();e++){
    //~ //cout << "Elem "<<e<<endl;
    //~ int nc = mesh.getElem(e)->getNodeCount();
    //~ if (nc==3){
      //~ vtkNew<vtkTriangle> cell;
      //~ for (int nn=0;nn<nc;nn++) {
        //~ cell->GetPointIds()->SetId(nn, mesh.getElem(e)->getNodeId(nn)-1); //EXTERNAL ID IS FROM 1

      //~ }
      //~ polys->InsertNextCell(cell);
    //~ } else if (mesh.getElem(e)->getNodeCount()==4){ //CHECK ALSO DIMENSION
      //~ vtkNew<vtkQuad> cell;
      //~ //cout << "gm mesh node "<<endl;
      //~ for (int nn=0;nn<nc;nn++) {
        //~ cell->GetPointIds()->SetId(nn, mesh.getElem(e)->getNodeId(nn)-1);//EXTERNAL ID IS FROM 1
        //~ cout <<  mesh.getElem(e)->getNodeId(nn) <<", ";
      //~ }
      //~ //cout <<endl;
      //~ polys->InsertNextCell(cell);
      //~ }
    //~ //cout <<endl;
  //~ }// if FEM 
  

 
 
  
  //~ //REUSE THIS
  //~ cout <<  "Setting data"<<endl;
  //~ // We now assign the pieces to the vtkPolyData.
  //~ mesh_pdata->SetPoints(points);
  //~ mesh_pdata->SetPolys(polys);
  
  //~ }


  //~ // Now we'll look at it.
  //~ cout << "Setting mapper "<<endl;
  //~ mesh_Mapper = vtkSmartPointer<vtkPolyDataMapper>::New(); 
  
  //~ mesh_Mapper->SetInputData(mesh_pdata);
  //~ mesh_Mapper->SetScalarRange(mesh_pdata->GetScalarRange());

  //~ mesh_actor = vtkSmartPointer<vtkActor>::New();
  //~ mesh_actor->SetMapper(mesh_Mapper);
  //~ cout << "Changing props"<<endl;


  //~ mesh_actor->GetProperty()->EdgeVisibilityOn ();
  //~ mesh_actor->GetProperty()->SetEdgeColor (0.0, 0.0, 0.0);
  //~ mesh_actor->Modified ();

  //~ //m_needs_actor = false; //Need stril to be added to renderer To Show
  //~ cout << "Created polydata from mesh"<<endl;
  //~ return EXIT_SUCCESS;    
//~ }

int GraphicMesh::createVTKPolyData(Mesh &mesh) {
    m_needs_polydata = false;
    m_needs_actor = true;
    mesh_actor = nullptr;
    mesh_pdata = nullptr;
  
    mesh_pdata = vtkSmartPointer<vtkPolyData>::New();
    points = vtkSmartPointer<vtkPoints>::New();
    vtkNew<vtkCellArray> cells;
    vtkNew<vtkFloatArray> scalars;

    std::cout << "Node count: " << mesh.getNodeCount() << std::endl;

    // Primera pasada: insertar todos los puntos
    for (int n = 0; n < mesh.getNodeCount(); n++) {
        std::array<float, 3> coords;
        for (int d = 0; d < 3; d++) {
            coords[d] = mesh.m_node[n]->getPos()[d];
        }
        points->InsertPoint(n, coords[0], coords[1], coords[2]);
        scalars->InsertTuple1(n, n);
    }

    // Segunda pasada: procesar elementos según su tipo
    if (mesh.getType() == FEM) {
        std::cout << "Mesh Type is FEM" << std::endl;
        
        bool has1DElements = false;
        bool has2DElements = false;
        bool has3DElements = false;

        for (int e = 0; e < mesh.getElemCount(); e++) {
            int nc = mesh.getElem(e)->getNodeCount();
            vtkSmartPointer<vtkCell> cell;

            // Determinar el tipo de elemento
            if (nc == 2) {
                // Elemento 1D (Línea)
                cell = vtkSmartPointer<vtkLine>::New();
                has1DElements = true;
            } else if (nc == 3) {
                // Elemento 2D (Triángulo)
                cell = vtkSmartPointer<vtkTriangle>::New();
                has2DElements = true;
            } else if (nc == 4) {
                // Podría ser Quad 2D o Tetraedro 3D - necesitamos verificar la dimensión
                // Asumimos que si la malla es FEM y tiene dimensión 2, es un Quad
                if (mesh.getDim() == 2) {
                    cell = vtkSmartPointer<vtkQuad>::New();
                    has2DElements = true;
                } else {
                    cell = vtkSmartPointer<vtkTetra>::New();
                    has3DElements = true;
                }
            } else if (nc == 8) {
                // Hexaedro 3D
                //cell = vtkSmartPointer<vtkHexahedron>::New();
                //has3DElements = true;
            } else {
                std::cout << "Unsupported element with " << nc << " nodes" << std::endl;
                continue;
            }

            // Establecer la conectividad de la celda
            for (int nn = 0; nn < nc; nn++) {
                // NOTA: Asumiendo que getNodeId() devuelve IDs basados en 1
                int nodeId = mesh.getElem(e)->getNodeId(nn) ;
                if (nodeId >= 0 && nodeId < mesh.getNodeCount()) {
                    cell->GetPointIds()->SetId(nn, nodeId);
                } else {
                    std::cerr << "Error: Invalid node ID " << nodeId + 1 
                              << " in element " << e << std::endl;
                }
            }

            cells->InsertNextCell(cell);
            
        }//Element loop

        // Configurar el polydata según los tipos de elementos presentes
        mesh_pdata->SetPoints(points);
        
        if (has2DElements) {
            mesh_pdata->SetPolys(cells);
        } else if (has1DElements) {
            mesh_pdata->SetLines(cells);
        } else if (has3DElements) {
            mesh_pdata->SetPolys(cells); // Para elementos 3D también usamos polys en VTK
        }

        mesh_pdata->GetPointData()->SetScalars(scalars);

    } else if (mesh.getType() == SPH) {
        // Manejo para mallas SPH (partículas)
        std::cout << "Mesh Type is SPH" << std::endl;
        //return createSPHVTKPolyData(mesh);
    } else {
        std::cerr << "Unknown mesh type" << std::endl;
        return EXIT_FAILURE;
    }

    // Configurar mapper y actor
    mesh_Mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mesh_Mapper->SetInputData(mesh_pdata);
    mesh_Mapper->SetScalarRange(mesh_pdata->GetScalarRange());

    mesh_actor = vtkSmartPointer<vtkActor>::New();
    mesh_actor->SetMapper(mesh_Mapper);

    vtkNew<vtkNamedColors> colors;
    mesh_actor->GetProperty()->SetColor(colors->GetColor3d("Silver").GetData());
    mesh_actor->GetProperty()->EdgeVisibilityOn();
    mesh_actor->GetProperty()->SetEdgeColor(0.0, 0.0, 0.0);
    
    // Mostrar wireframe para elementos 2D/3D
    if (mesh.getType() == FEM && mesh.getDim() >= 2) {
        mesh_actor->GetProperty()->SetRepresentationToWireframe();
    }
    
    mesh_actor->Modified();

    std::cout << "Created polydata from mesh with " << mesh.getNodeCount() 
              << " nodes and " << mesh.getElemCount() << " elements" << std::endl;
    return EXIT_SUCCESS;
}

int GraphicSPHMesh::createVTKPolyData(Mesh &mesh){
  cout << "Creating SPH Mesh "<<endl;
  mesh_pdata = vtkSmartPointer<vtkPolyData>::New();
  
  setPoints(mesh);
  m_radius = 0.5;

  if (mesh.getType() == SPH){
    cout << "Creating SPH Graphic Mesh "<<endl;
    vtkNew<vtkSphereSource> cubeSource;
    cubeSource->SetRadius(m_radius);
    m_glyph3D = vtkSmartPointer<vtkGlyph3D>::New();
    
    m_glyph3D->SetSourceConnection(cubeSource->GetOutputPort());
    //m_glyph3D->SetInputData(polydata);
    m_glyph3D->Update();  
    

        
  }// if FEM 
  else {
    cout << "ERROR. Mesh should be of type SPH"<<endl;
  }
  

 
 
  
  //REUSE THIS
  cout <<  "Setting data"<<endl;
  // We now assign the pieces to the vtkPolyData.
  mesh_pdata->SetPoints(points);
  
  m_glyph3D->SetInputData(mesh_pdata);
  //mesh_pdata->SetPolys(polys);
  
  

  //TODO: REUSE THIS
  // Now we'll look at it.
  cout << "--Setting mapper "<<endl;
  mesh_Mapper = vtkSmartPointer<vtkPolyDataMapper>::New(); 
  
  mesh_Mapper->SetInputData(mesh_pdata);
  mesh_Mapper->SetScalarRange(mesh_pdata->GetScalarRange());

  mesh_Mapper->SetInputConnection(m_glyph3D->GetOutputPort());

  mesh_actor = vtkSmartPointer<vtkActor>::New();
  mesh_actor->SetMapper(mesh_Mapper);
  cout << "Changing props"<<endl;


  mesh_actor->GetProperty()->EdgeVisibilityOn ();
  mesh_actor->GetProperty()->SetEdgeColor (0.0, 0.0, 0.0);
  mesh_actor->Modified ();
  //// IF VTK later than 9.3?
  //mesh_actor->GetProperty()->SetDepthTest(false);
  mesh_actor->GetMapper()->SetResolveCoincidentTopology(VTK_RESOLVE_OFF);
  
  m_needs_actor = true; //To Show

  cout << "Created polydata from empty"<<endl;    
  return EXIT_SUCCESS;    
}


GraphicMesh::GraphicMesh(Mesh *mesh):
m_mesh(mesh)
{
    m_needs_actor = true;
    mesh_actor = nullptr;
    m_needs_viewer = true;
    m_viewer = nullptr;
    createVTKPolyData(*mesh);
  
}

GraphicSPHMesh::GraphicSPHMesh(Mesh *mesh)
{

/*
  vtkNew<vtkGlyph3D> glyph3D;
  glyph3D->SetSourceConnection(cubeSource->GetOutputPort());
  glyph3D->SetInputData(polydata);
  glyph3D->Update();  
  
  mapper->SetInputConnection(glyph3D->GetOutputPort());
*/
}

GraphicSPHMesh::GraphicSPHMesh()
{
  
  
  
}


    void GraphicMesh::SetTransform(vtkSmartPointer<vtkTransform> transform) {
        m_transform = transform;
        if (mesh_actor) {
            mesh_actor->SetUserTransform(m_transform);
        }
    }
    

    
    void GraphicMesh::UpdateMeshPosition() {
        if (!m_mesh || !m_transform) return;
        
        // Actualizar nodos del mesh según la transformación
        double currentPos[3];
        m_transform->GetPosition(currentPos);
        
        // Aquí actualizas los nodos reales del Mesh
        for (int i = 0; i < m_mesh->getNodeCount(); i++) {
            Node* node = m_mesh->getNode(i);
            Vector3f originalPos ;//= node->getOriginalPos();
            Vector3f newPos = originalPos + Vector3f(currentPos[0], currentPos[1], currentPos[2]);
            //node->setPos(newPos);
        }
    }
    
    Vector3f GraphicMesh::GetPosition() const {
        if (!m_transform) return Vector3f(0, 0, 0);
        double pos[3];
        m_transform->GetPosition(pos);
        return Vector3f(pos[0], pos[1], pos[2]);
    }
    
    void GraphicMesh::SetPosition(const Vector3f& position) {
        if (!m_transform) m_transform = vtkSmartPointer<vtkTransform>::New();
        m_transform->Identity();
        m_transform->Translate(position.x, position.y, position.z);
        if (mesh_actor) {
            mesh_actor->SetUserTransform(m_transform);
        }
        UpdateMeshPosition();
    } 
