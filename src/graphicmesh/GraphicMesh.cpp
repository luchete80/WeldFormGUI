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

///// IF IMPORTING FROM GEOM
/// ASOCIATE WITH GEOM!
int GraphicMesh::createVTKPolyData() {


  //m_needs_update = true;
  // Print the model name and dimension:
  std::string name;
  gmsh::model::getCurrent(name);
  std::cout << "Model " << name << " (" << gmsh::model::getDimension()
            << "D)\n";

  //GET PART!!!

  // Geometrical data is made of elementary model `entities', called `points'
  // (entities of dimension 0), `curves' (entities of dimension 1), `surfaces'
  // (entities of dimension 2) and `volumes' (entities of dimension 3). As we
  // have seen in the other C++ tutorials, elementary model entities are
  // identified by their dimension and by a `tag': a strictly positive
  // identification number. Model entities can be either CAD entities (from the
  // built-in `geo' kernel or from the OpenCASCADE `occ' kernel) or `discrete'
  // entities (defined by a mesh). `Physical groups' are collections of model
  // entities and are also identified by their dimension and by a tag.

  // Get all the elementary entities in the model, as a vector of (dimension,
  // tag) pairs:
  std::vector<std::pair<int, int> > entities;
  gmsh::model::getEntities(entities);


  std::vector <std::array<float,3>> pts; 

  //std::vector <std::array<int,3>> elnodes; 
  std::vector <std::vector <int> > elnodes; 
  std::map< int,int > nodetagpos;
  int nodecount =0;

  cout << "Entity count "<<entities.size()<<endl;
  int dim_count[] = {0,0,0};
  for(auto e : entities) {

    int dim = e.first, tag = e.second;
    dim_count[dim]++;

    // Get the mesh nodes for the entity (dim, tag):
    std::vector<std::size_t> nodeTags;
    std::vector<double> nodeCoords, nodeParams;
    gmsh::model::mesh::getNodes(nodeTags, nodeCoords, nodeParams, dim, tag);


    std::ofstream myfile;
    myfile.open ("example.csv");
    //if (dim ==2){

      //cout << "Node coords size "<<nodeCoords.size()<<endl; 
      
      for (int n=0;n<nodeCoords.size()/3;n++){
          nodecount++;
        //}
      }
    //}//dim
  }
  cout << "Overall node count "<<nodecount<<endl;
  
  int nc=0;
  pts.resize(nodecount+1);
  cout << "Entities by dimension: "<<dim_count[0]<< ", "<<dim_count[1]<< ", "<<dim_count[2]<< endl;
  gmsh::model::getEntities(entities);
  for(auto e : entities) {
    cout<<" ---- \n"<<endl;
    // Dimension and tag of the entity:
    int dim = e.first, tag = e.second;

    // Mesh data is made of `elements' (points, lines, triangles, ...), defined
    // by an ordered list of their `nodes'. Elements and nodes are identified by
    // `tags' as well (strictly positive identification numbers), and are stored
    // ("classified") in the model entity they discretize. Tags for elements and
    // nodes are globally unique (and not only per dimension, like entities).

    // A model entity of dimension 0 (a geometrical point) will contain a mesh
    // element of type point, as well as a mesh node. A model curve will contain
    // line elements as well as its interior nodes, while its boundary nodes
    // will be stored in the bounding model points. A model surface will contain
    // triangular and/or quadrangular elements and all the nodes not classified
    // on its boundary or on its embedded entities. A model volume will contain
    // tetrahedra, hexahedra, etc. and all the nodes not classified on its
    // boundary or on its embedded entities.

    // Get the mesh nodes for the entity (dim, tag):
    std::vector<std::size_t> nodeTags;
    std::vector<double> nodeCoords, nodeParams;
    gmsh::model::mesh::getNodes(nodeTags, nodeCoords, nodeParams, dim, tag);
    //cout << "Node coords size "<<endl;
    //cout << "Node Coords "<<nodeCoords[0]<<", " << nodeCoords[1]<<", "<<nodeCoords[2]<<endl;

    // Get the mesh elements for the entity (dim, tag):
    std::vector<int> elemTypes;
    std::vector<std::vector<std::size_t> > elemTags, elemNodeTags;
    gmsh::model::mesh::getElements(elemTypes, elemTags, elemNodeTags, dim, tag);

    std::ofstream myfile;
    myfile.open ("example.csv");
    
    // * Number of mesh nodes and elements:
    int numElem = 0;
    cout << "Element tags size "<<elemTags.size()<<endl;
    for(auto &tags : elemTags) numElem += tags.size();
    
      std::cout << " - Mesh has " << nodeTags.size() << " nodes and " << numElem
              << " elements\n";
      cout << "Node coords size "<<nodeCoords.size()<<endl; 
      
      for (int n=0;n<nodeCoords.size()/3;n++){
        for (int d=0;d<3;d++){
          cout << "Node "<<n<<": "<<nodeCoords[3*n+d]<<", "<<endl;
        }
        nodetagpos[nodeTags[n]]=nc;
        
        cout << "Node pos local"<<n << " and global "<<nc<<" has tag "<<nodeTags[n]<<endl;
          //test[n][d]= nodeCoords[3*n+d];
          //float coords[3];
          std::array <float,3> coords;
          for (int d=0;d<3;d++) coords[d] = nodeCoords[3*n+d];
          // IF REAL POSITIONS
          //pts.push_back(coords);

          if (nodeTags[n]<nodecount)
            pts[nodeTags[n]]=coords;
          else
            cout << "ERROR IN NODE "<<nodeTags[n]<<endl;

          nc++;
        //}
      }
      cout << "Nodes inside nodeTags"<<endl;
      
      for (auto n: nodeTags){
        cout << n<<" ";
      }   
      cout << endl;
      
    if (dim ==1){ 
      
      cout << "Generating graphic mesh 1D "<<endl;
        for(int ne=0;ne<elemNodeTags[0].size()/3;ne++)   { 
          std::vector <int> conn; conn.resize(2);
          cout << "Local "  << elemNodeTags[0][2*ne] << ", "<<elemNodeTags[0][2*ne+1] <<endl;
          cout << "Global " << nodetagpos[elemNodeTags[0][3*ne]] <<", "<< nodetagpos[elemNodeTags[0][3*ne+1]] << endl;
          for (int d=0;d<2;d++) {
            conn[d] = elemNodeTags[0][2*ne+d];
            
            //If defined with gmsh positions 
            //conn[d] = nodetagpos[elemNodeTags[0][3*ne+d]] ;/*elemNodeTags[0][3*ne+d];

          }
          elnodes.push_back(conn);
        }      
        
      }else if (dim ==2){
      for(auto &tags : elemTags){ 
        cout << "Element inside tags "<<endl;
        for (int t=0;t<tags.size();t++)
          cout <<tags[t]<<" ";
        cout << endl;
        
        cout << endl<<"Element nodes size"<< elemNodeTags.size()<<", "<<elemNodeTags[0].size()<<endl;
        for(auto ne: elemNodeTags[0])   { 
          cout << ne << " ";//numElem += tags.size();          
        }
        cout << endl;
        
        for(int ne=0;ne<elemNodeTags[0].size()/3;ne++)   { 
          //std::array <int,3> conn;
          std::vector<int> conn;
          conn.resize(3);
          //cout << "Local "  << elemNodeTags[0][3*ne] << ", "<<elemNodeTags[0][3*ne+1] << ", "<<elemNodeTags[0][3*ne+2] <<endl;
          //cout << "Global " << nodetagpos[elemNodeTags[0][3*ne]] <<", "<< nodetagpos[elemNodeTags[0][3*ne+1]]<<", " << nodetagpos[elemNodeTags[0][3*ne+2]] <<endl;
          for (int d=0;d<3;d++) {
            conn[d] = elemNodeTags[0][3*ne+d];
            
            //If defined with gmsh positions 
            //conn[d] = nodetagpos[elemNodeTags[0][3*ne+d]] ;/*elemNodeTags[0][3*ne+d];*/

          }
          elnodes.push_back(conn);
        }
      }//elem tags
    }// dim 2


    // * List all types of elements making up the mesh of the entity:
    for(auto elemType : elemTypes) {
      std::string name;
      int d, order, numv, numpv;
      std::vector<double> param;
      gmsh::model::mesh::getElementProperties(elemType, name, d, order, numv,
                                              param, numpv);
      std::cout << " - Element type: " << name << ", order " << order << "\n";
      std::cout << "   with " << numv << " nodes in param coord: (";
      for(auto p : param) std::cout << p << " ";
      std::cout << ")\n";
    }
    cout << "elem tag size: "<<elemTags.size()<<", element nodetag size "<<elemNodeTags.size()<<endl; 
    //for (auto enode : elemNodeTags[0]){
      //cout << elemNodeTags[0][enode]<<endl;}
        
    
  }//entities
  
  cout << "Element Nodes size "<<elnodes.size()<<endl;
  
  //IF QUAD
  //https://examples.vtk.org/site/Cxx/GeometricObjects/Quad/
  // Create a quad on the four points
/*
  vtkNew<vtkQuad> quad;
  quad->GetPointIds()->SetId(0, 0);
  quad->GetPointIds()->SetId(1, 1);
  quad->GetPointIds()->SetId(2, 2);
  quad->GetPointIds()->SetId(3, 3);

  // Create a cell array to store the quad in
  vtkNew<vtkCellArray> quads;
  quads->InsertNextCell(quad);
*/
  vtkNew<vtkTriangle> quad;
  
  vtkNew<vtkNamedColors> colors;

  /*
  std::array<std::array<double, 3>, 8> pts = {{{{0, 0, 0}},
                                               {{1, 0, 0}},
                                               {{1, 1, 0}},
                                               {{0, 1, 0}},
                                               {{0, 0, 1}},
                                               {{1, 0, 1}},
                                               {{1, 1, 1}},
                                               {{0, 1, 1}}}};
  */
  // The ordering of the corner points on each face.
  std::array<std::array<vtkIdType, 4>, 6> ordering = {{{{0, 3, 2, 1}},
                                                       {{4, 5, 6, 7}},
                                                       {{0, 1, 5, 4}},
                                                       {{1, 2, 6, 5}},
                                                       {{2, 3, 7, 6}},
                                                       {{3, 0, 4, 7}}}};

  // We'll create the building blocks of polydata including data attributes.
  mesh_pdata = vtkSmartPointer<vtkPolyData>::New();
  
  points = vtkSmartPointer<vtkPoints>::New();
  polys  = vtkSmartPointer<vtkCellArray>::New();
  scalars = vtkSmartPointer<vtkFloatArray>::New();

  // Load the point, cell, and data attributes.
  for (auto i = 0ul; i < pts.size(); ++i)
  {
    points->InsertPoint(i, pts[i].data());
    scalars->InsertTuple1(i, i);
  }
  
  for (auto&& i : ordering)
  {
    //polys->InsertNextCell(vtkIdType(i.size()), i.data());
  }
  
  //TEMPLATIZE
  for (int e=0;e<elnodes.size();e++){
    int ne = elnodes[e].size();
    if (ne==3){
      vtkNew<vtkTriangle> tri;
      for (int nn=0;nn<elnodes[e].size();nn++) {
        tri->GetPointIds()->SetId(nn, elnodes[e][nn]);
        cout <<elnodes[e][nn]<<", ";
      }
      cout <<endl;
      polys->InsertNextCell(tri);
    } else if (ne ==2){
      //cout << "Inserting element 1D"<<endl; 
      vtkNew<vtkLine> tri;
      for (int nn=0;nn<elnodes[e].size();nn++) {
        tri->GetPointIds()->SetId(nn, elnodes[e][nn]);
        cout <<elnodes[e][nn]<<", ";
      }
      cout <<endl;
      polys->InsertNextCell(tri);      
      
      }
  }

  cout <<  "Setting data"<<endl;
  // We now assign the pieces to the vtkPolyData.
  mesh_pdata->SetPoints(points);
  mesh_pdata->SetPolys(polys);
  mesh_pdata->GetPointData()->SetScalars(scalars);

  // Now we'll look at it.
  cout << "--Setting mapper "<<endl;
  mesh_Mapper = vtkSmartPointer<vtkPolyDataMapper>::New(); 
  
  mesh_Mapper->SetInputData(mesh_pdata);
  mesh_Mapper->SetScalarRange(mesh_pdata->GetScalarRange());
  //vtkNew<vtkActor> cubeActor;
  mesh_actor = vtkSmartPointer<vtkActor>::New();
  mesh_actor->SetMapper(mesh_Mapper);
  cout << "Changing props"<<endl;
  mesh_actor->GetProperty()->SetColor(colors->GetColor3d("Silver").GetData());

  mesh_actor->GetProperty()->EdgeVisibilityOn ();
  mesh_actor->GetProperty()->SetEdgeColor (0.0, 0.0, 0.0);
  mesh_actor->GetProperty()->SetRepresentationToWireframe();
  mesh_actor->Modified ();
  
  //m_needs_actor = false; //NEEDS TO BE ADDED!
  cout << "Created polydata from empty"<<endl;
  
  return EXIT_SUCCESS;
}

int GraphicMesh::createVTKPolyData(Mesh &mesh)
{
  m_needs_polydata = false;
  m_needs_actor = true; //needs actor to be shown
  mesh_actor = nullptr;
  mesh_pdata = nullptr;
  
  mesh_pdata = vtkSmartPointer<vtkPolyData>::New();
  
  std::vector <std::array<float,3>> pts; 
  
  points = vtkSmartPointer<vtkPoints>::New();
  cout << "Node count "<<mesh.getNodeCount()<<endl;
  for (int n=0;n<mesh.getNodeCount();n++){
    cout << "Node "<<n<<endl;
          std::array <float,3> coords;
          for (int d=0;d<3;d++) coords[d] = mesh.m_node[n]->getPos()[d];
          // IF REAL POSITIONS
          pts.push_back(coords);
    for (int d=0; d<3; d++) {
        coords[d] = mesh.m_node[n]->getPos()[d];
        cout << "Raw coord " << d << ": " << coords[d] << endl;
    }

    points->InsertPoint(n, coords.data());
    // Verify what was actually inserted
    double inserted[3];
    points->GetPoint(n, inserted);
    cout << "Inserted coord: " 
         << inserted[0] << ", " << inserted[1] << ", " << inserted[2] << endl;
    }

  // cout << "Inserting nodes"<<endl;
  // for (auto i = 0ul; i < pts.size(); ++i)
  // {
    // //cout << "Node"<<i<<endl;
    // points->InsertPoint(i, pts[i].data());
    // //scalars->InsertTuple1(i, i);
  // }
  // cout << "Done"<<endl;

  if (mesh.getType()==FEM){
  cout << "Mesh Type is FEM"<<endl;
  polys  = vtkSmartPointer<vtkCellArray>::New();

  for (int e=0;e<mesh.getElemCount();e++){
    //cout << "Elem "<<e<<endl;
    int nc = mesh.getElem(e)->getNodeCount();
    if (nc==3){
      vtkNew<vtkTriangle> cell;
      for (int nn=0;nn<nc;nn++) {
        cell->GetPointIds()->SetId(nn, mesh.getElem(e)->getNodeId(nn)-1); //EXTERNAL ID IS FROM 1

      }
      polys->InsertNextCell(cell);
    } else if (mesh.getElem(e)->getNodeCount()==4){ //CHECK ALSO DIMENSION
      vtkNew<vtkQuad> cell;
      //cout << "gm mesh node "<<endl;
      for (int nn=0;nn<nc;nn++) {
        cell->GetPointIds()->SetId(nn, mesh.getElem(e)->getNodeId(nn)-1);//EXTERNAL ID IS FROM 1
        cout <<  mesh.getElem(e)->getNodeId(nn) <<", ";
      }
      //cout <<endl;
      polys->InsertNextCell(cell);
      }
    //cout <<endl;
  }// if FEM 
  

 
 
  
  //REUSE THIS
  cout <<  "Setting data"<<endl;
  // We now assign the pieces to the vtkPolyData.
  mesh_pdata->SetPoints(points);
  mesh_pdata->SetPolys(polys);
  
  }


  // Now we'll look at it.
  cout << "SSSSSetting mapper "<<endl;
  mesh_Mapper = vtkSmartPointer<vtkPolyDataMapper>::New(); 
  
  mesh_Mapper->SetInputData(mesh_pdata);
  mesh_Mapper->SetScalarRange(mesh_pdata->GetScalarRange());

  mesh_actor = vtkSmartPointer<vtkActor>::New();
  mesh_actor->SetMapper(mesh_Mapper);
  cout << "Changing props"<<endl;


  mesh_actor->GetProperty()->EdgeVisibilityOn ();
  mesh_actor->GetProperty()->SetEdgeColor (0.0, 0.0, 0.0);
  mesh_actor->Modified ();

  //m_needs_actor = false; //Need stril to be added to renderer To Show
  cout << "Created polydata from mesh"<<endl;
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
