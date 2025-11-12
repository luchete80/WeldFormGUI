#include "Mesh.h"
#include "Node.h"

#include <iostream>
#include <map>

#include <gmsh.h>
#include <array>
using namespace std;

/*
void initValues(  std::vector <Node*>    m_node, //LOCATED ON MODEL SPACE!!!!
                    std::vector <int>      elnod){
  

}
*/

void Mesh::addNode(double x, double y, double z, int id){
  int max = 0;
  if (id ==-1){
    for (int i=0;i<m_node.size();i++){
        if (m_node[i]->getId()>max)
          max = m_node[i]->getId();
    }
    id = max+1;
  }
  cout << "adding Node "<<"id "<<id << "x: "<<x<<", y: "<<y<<", z: "<<z<<endl;
  m_node.push_back(new Node(x,y,z,id));
  m_node_count++;
}
  
void Mesh::addNode(Node *node){
  m_node.push_back(node);
}

void Mesh::addElement(Element *el, bool alloc){
    m_elem.push_back(new Element(*el));

}

void Mesh::addQuad(int v0, int v1, int v2, int v3, int id){
  
  if (id ==-1){
  int max=0;
  for (int i=0;i<m_elem.size();i++){
      if (m_elem[i]->getId()>max)
        max = m_node[i]->getId();
  }
    id = max+1;
  }
//CHACK IF ELEMENT DOES NOT EXIST
  m_elem.push_back(new Quad(m_node[v0],m_node[v1],m_node[v2],m_node[v3]));
  m_elem_count++;
}

void Mesh::assignValues(  std::vector <Node*>    n, //LOCATED ON MODEL SPACE!!!!
                    std::vector <Element*> e){
  m_node.resize(n.size());
  for (int nn=0;nn<n.size(); nn++) m_node[nn]=n[nn];
  m_node_count = n.size();
  
  m_elem.resize(e.size());
  for (int nn=0;nn<e.size(); nn++) {
    //cout << "element nodecount " << e[nn]->m_node.size()<<endl;
    // for (int ne=0;ne<8;ne++)
      // cout << "eleement node id "<< e[nn]->getNodeId(ne)<<endl;
    m_elem[nn]=e[nn];
  }
  m_elem_count = e.size();
}

const Vector3f& Mesh::getNodePos(const int &i)const{
  return m_node[i]->getPos();
}


void Mesh::addPlane(double x0, double y0, double lx, double ly, double d){
  addBoxLength(Vector3f(x0,y0,0.0),Vector3f(lx,ly,0.0), d/2.0);
  
  }
  
  
// #include <glm/gtc/matrix_transform.hpp>
void Mesh::addBoxLength(Vector3f V, Vector3f L, double r){
    Vector3f Xp;
    
    int p, nnodz;
    int nel[3];
    m_dim = 2;
    
    if (L[2] > 0.0) m_dim = 3;
    cout << "Dimension set to "<<m_dim<<endl;
    
    nel[0] = (int)(L[0]/(2.0*r));
    nel[1] = (int)(L[1]/(2.0*r));
    nel[2] = (int)(L[1]/(2.0*r));
    
    cout << "Nel x: "<<nel[0]<<", y "<<nel[1]<<endl;
    
    // // m_gp_count = 1;
    // // if (m_dim == 2){
      // // nel[2] = 1;
      // // m_nodxelem = 4;
      // // if (!red_int) m_gp_count = 4;
    // // } else {
      // // nel[2] = (int)(L.z/(2.0*r));
      // // m_nodxelem = 8;
      // // if (!red_int) m_gp_count = 8; 
    // // }
    

    // Xp[2] = V[2] ;
    

    // // // write (*,*) "Creating Mesh ...", "Elements ", neL.y, ", ",neL.z
  
  if (m_dim == 2){
    m_elem_count = nel[0]*nel[1];
    m_node_count = (nel[0] +1) * (nel[1]+1);
  } else { 
    m_elem_count = nel[0]*nel[1]*nel[2];
    m_node_count = (nel[0] +1) * (nel[1]+1) * (nel[2]+1);
  }
  cout << "Mesh created. Element count: "<< nel[0]<<", "<<nel[1]<<", "<<nel[2]<<endl;

  // // //thisAllocateNodes((nel[0] +1) * (nel[1]+1) * (nel[2]+1));
    // // // print *, "Element count in XYZ: ", nel(:)
    // // // write (*,*) "Box Node count ", node_count

	// // this->SetDimension(nc,ne);	 //AFTER CREATING DOMAIN
  // // cout << "Mesh generated. Node count: " << nc<<". Element count: "<<ne<<endl;
  // // cout << "Dimension is: "<<m_dim<<endl;
  // // //SPH::Domain	dom;
	// // //vector_t *x =  (vector_t *)malloc(dom.Particles.size());
	// // vector_t *x_H =  new vector_t [m_node_count];


	// // //int size = dom.Particles.size() * sizeof(vector_t);
	// // cout << "Copying to device..."<<endl;

    if (m_dim == 2) {    
    // cout << "Box Particle Count is " << m_node_count <<endl;
    p = 0;
    Xp.x = 0.0;
    //for (int k = 0; k < (nel[2] +1);k++) {
      Xp.y= V.y;
      for (int j = 0; j < (nel[1] +1);j++){
        Xp.x = V.x;
        for (int i = 0; i < (nel[0] +1);i++){
					//m_node.push_back(new Node(Xp));
					//x_H[p] = Xp;
          //nod%x(p,:) = Xp(:);
          cout << "node " << p <<"X: "<<Xp[0]<<"Y: "<<Xp.y<<"Z: "<<Xp.z<<endl;
          m_node.push_back(new Node(Xp.x,Xp.y,0.0,p));
          p++;
          Xp.x += 2.0 * r;
        }
        Xp.y += 2.0 * r;
      }// 
      // Xp.z = Xp.z + 2 * r;
    } else {
      Xp.z = 0.0;
      p = 0;
      for (int k = 0; k < (nel[2] +1);k++){
        Xp.x = 0.0;
        //for (int k = 0; k < (nel[2] +1);k++) {
        Xp.y= V.y;
        for (int j = 0; j < (nel[1] +1);j++){
          Xp.x = V.x;
          for (int i = 0; i < (nel[0] +1);i++){
            //m_node.push_back(new Node(Xp));
            //x_H[p] = Xp;
            //nod%x(p,:) = Xp(:);
            cout << "node " << p <<"X: "<<Xp[0]<<"Y: "<<Xp.y<<"Z: "<<Xp.z<<endl;
            m_node.push_back(new Node(Xp.x,Xp.y,Xp.z,p));
            p++;
            Xp.x += 2.0 * r;
          }
          Xp.y += 2.0 * r;
        }// 
         Xp.z += 2 * r;     
      }//for k
      
    }
    // //cout <<"m_node size"<<m_node.size()<<endl;
    // } 
		// memcpy_t(this->x, x_H, sizeof(vector_t) * m_node_count);    

    // // // !! ALLOCATE ELEMENTS
    // // // !! DIMENSION = 2
    // int gp = 1;
    // if (m_dim == 2) {
      // // if (redint .eqv. .False.) then
        // // gp = 4
      // // end if 
      // //call AllocateElements(neL.y * neL.z,gp) !!!!REDUCED INTEGRATION
    // } else {
      // // if (redint .eqv. .False.) then
        // // gp = 8
      // // end if 
      // // call AllocateElements(neL.y * neL.z*nel(3),gp) 
    // }

		//int *elnod_h       = new int [m_elem_count * m_nodxelem]; //Flattened

    // int *nodel_count_h  = new int [m_node_count];
    // int *nodel_offset_h = new int [m_node_count];
    
		int ex, ey, ez;
		std::vector <Node*> n;
    if (m_dim == 2) {
			n.resize(4);
      int ei = 0;
      for (int ey = 0; ey < nel[1];ey++){
        for (int ex = 0; ex < nel[0];ex++){
        int iv[4];
        //elnod_h[ei  ] = (nel[0]+1)*ey + ex;        elnod_h[ei+1] = (nel[0]+1)*ey + ex+1;
        //elnod_h[ei+2] = (nel[0]+1)*(ey+1) + ex+1;  elnod_h[ei+3] = (nel[0]+1)*(ey+1) + ex;

        n[0] = m_node[(nel[0]+1)*ey + ex];        n[1] = m_node[(nel[0]+1)*ey + ex+1];
        n[2] = m_node[(nel[0]+1)*(ey+1) + ex+1];  n[3] = m_node[(nel[0]+1)*(ey+1) + ex];
        m_elem.push_back(new Quad(n));
				 // for (int i=0;i<m_nodxelem;i++)cout << elnod_h[ei+i]<<", ";
					// cout << "Nel x : "<<nel[0]<<endl;
					// cout << "nodes "<<endl;
					// ei += m_nodxelem;
					}
       } 
    } else { //dim: 3
      int ei = 0; //ELEMENT INTERNAL NODE (GLOBAL INDEX)
      n.resize(8);
      int nnodz = (nel[0]+1)*(nel[1]+1);
      for (int ez = 0; ez < nel[2];ez++){
        for (int ey = 0; ey < nel[1];ey++){
          for (int ex = 0; ex < nel[0];ex++){
            cout << "elem 1"<< endl;
            int iv[8];
            int nb1 = nnodz*ez + (nel[0]+1)*ey + ex;
            int nb2 = nnodz*ez + (nel[0]+1)*(ey+1) + ex;
            
            n[0] = m_node[nb1  ];                        
            n[1] = m_node[nb1+1];                    
            n[2] = m_node[nb2+1];                    
            n[3] = m_node[nb2  ];                      
            
            n[4] = m_node[nb1 + nnodz    ];      
            n[5] = m_node[nb1 + nnodz + 1];    
            n[6] = m_node[nb2 + nnodz + 1];   
            n[7] = m_node[nb2 + nnodz    ];         
            //cout << "largest ind "<<nb2 + nnodz + 1<<endl;
            // for (int i=0;i<8;i++)
              // cout << n[i]->getId()<<", ";
            // cout <<endl;
            m_elem.push_back(new Element(n));
            
             
             // for (int i=0;i<m_nodxelem;i++)cout << elnod_h[ei+i]<<", ";
             // ei += m_nodxelem;

          }
        } 
      }
		}//if dim 

    // // //cudaMalloc((void **)&m_elnod, m_elem_count * m_nodxelem * sizeof (int));	
    // // malloc_t(m_elnod, unsigned int, m_elem_count * m_nodxelem);
		// // memcpy_t(this->m_elnod, elnod_h, sizeof(int) * m_elem_count * m_nodxelem);    
    
    // // //cudaMalloc(&m_jacob,m_elem_count * sizeof(Matrix ));
    // // malloc_t(m_jacob, Matrix, m_elem_count );
    
    // // //////////////////// ELEMENT SHARED BY NODES (FOR PARALLEL NODAL MODE ASSEMBLY) ///////////////////////////////
    // int nodel_tot = 0;
    // for (int n=0;n<m_node_count;n++){
      // nodel_offset_h[n] = nodel_tot;
      // nodel_tot        += nodel_count_h[n];
      // cout << "Node "<< n << " Shared elements: "<<nodel_count_h[n]<<endl;

    // }
    // // cout << "Size of Nodal shared Elements vector "<< nodel_tot<<endl;
		// // int *nodel_h       = new int [nodel_tot];          //ASSUMED EACH NODE SHARES 8 ELEMENT
    // // int *nodel_loc_h   = new int [nodel_tot];          //ASSUMED EACH NODE SHARES 8 ELEMENT    
    
    // // for (int n=0;n<m_node_count;n++)  nodel_count_h[n] = 0;    
    // // for (int e=0;e<m_elem_count;e++){
      // // int offset = m_nodxelem * e;
      // // for (int ne=0;ne<m_nodxelem;ne++){
        // // int n = elnod_h[offset+ne];
        
        // // nodel_h     [nodel_offset_h[n] + nodel_count_h[n]] = e;
        // // nodel_loc_h [nodel_offset_h[n] + nodel_count_h[n]] = ne;
        
        // // nodel_count_h[n]++;
      // // }//nod x elem 
    // // }

    // // // cudaMalloc((void **)&m_nodel,     nodel_tot * sizeof (int));
    // // // cudaMalloc((void **)&m_nodel_loc, nodel_tot * sizeof (int));
    
    // // malloc_t (m_nodel,        int,nodel_tot);
    // // malloc_t (m_nodel_loc,    int,nodel_tot);
    
    // // malloc_t (m_nodel_offset, int, m_node_count);
    // // malloc_t (m_nodel_count,  int, m_node_count);
    
    // // //THIS IS ONLY FOR COMPLETENESS IN CASE OF CPU, SHOULD BE BETTER TO WRITE ON FINALL ARRAY
		// // memcpy_t(this->m_nodel,         nodel_h,        sizeof(int) * nodel_tot); 
		// // memcpy_t(this->m_nodel_loc,     nodel_loc_h,    sizeof(int) * nodel_tot);
		 
		// // memcpy_t(this->m_nodel_offset,  nodel_offset_h, sizeof(int) * m_node_count);  //OFFSET FOR PREVIOUS ARRAYS
		// // memcpy_t(this->m_nodel_count,    nodel_count_h, sizeof(int) * m_node_count);  //OFFSET FOR PREVIOUS ARRAYS
		    
    // // // ///// TESTING
    // // for (int n=0;n<m_node_count;n++){
      // // cout << "M node offset:"<<nodel_offset_h[n];
      // // cout << "Node  "<< n << " Elements"<<endl;
      // // for (int ne=0;ne<nodel_count_h[n];ne++) cout << nodel_h[nodel_offset_h[n]+ne]<<", ";
      // // cout << endl;
      // // cout << "Node  "<< n << " Elements Internal Node"<<endl;
      // // for (int ne=0;ne<nodel_count_h[n];ne++) cout << nodel_loc_h[nodel_offset_h[n]+ne]<<", ";
      // // cout << endl;
    // // }
    
  

		
		// // delete [] /*elnod_h, */nodel_count_h, nodel_h, nodel_loc_h,nodel_offset_h;
  
} // ADD BOX LENGTH


//~ void Mesh::genFromGmshModel() {

  //~ // Print the model name and dimension:
  //~ std::string name;
  //~ gmsh::model::getCurrent(name);
  //~ std::cout << "Model " << name << " (" << gmsh::model::getDimension()
            //~ << "D)\n";

  //~ //GET PART!!!

  //~ // Geometrical data is made of elementary model `entities', called `points'
  //~ // (entities of dimension 0), `curves' (entities of dimension 1), `surfaces'
  //~ // (entities of dimension 2) and `volumes' (entities of dimension 3). As we
  //~ // have seen in the other C++ tutorials, elementary model entities are
  //~ // identified by their dimension and by a `tag': a strictly positive
  //~ // identification number. Model entities can be either CAD entities (from the
  //~ // built-in `geo' kernel or from the OpenCASCADE `occ' kernel) or `discrete'
  //~ // entities (defined by a mesh). `Physical groups' are collections of model
  //~ // entities and are also identified by their dimension and by a tag.

  //~ // Get all the elementary entities in the model, as a vector of (dimension,
  //~ // tag) pairs:
  //~ std::vector<std::pair<int, int> > entities;
  //~ gmsh::model::getEntities(entities);
  //~ cout << "Entity count "<<entities.size()<<endl;


  //~ std::vector <std::array<float,3>> pts; 

  //~ //std::vector <std::array<int,3>> elnodes; 
  //~ std::vector <std::vector <int> > elnodes; 
  //~ std::map< int,int > nodetagpos;
  //~ int nodecount =0;

  //~ int dim_count[] = {0,0,0,0};
  //~ for(auto e : entities) {
    //~ int dim = e.first, tag = e.second;
    //~ dim_count[dim]++;    
  //~ }
  
  //~ cout << "DIM COUNT "<<endl;
  //~ cout << dim_count[0]<<", "<< dim_count[1]<<", "<< dim_count[2]<<", "<< dim_count[3]<<", "<<endl;
  //~ int max_dim=0;
  //~ for (int i=0;i<4;i++)
    //~ if (dim_count[i]>0) 
      //~ max_dim=i;
  //~ cout << "Max dim "<<max_dim<<endl;
  
  //~ for(auto e : entities) {

    //~ int dim = e.first, tag = e.second;

    //~ // Get the mesh nodes for the entity (dim, tag):
    //~ std::vector<std::size_t> nodeTags;
    //~ std::vector<double> nodeCoords, nodeParams;
    //~ gmsh::model::mesh::getNodes(nodeTags, nodeCoords, nodeParams, dim, tag);


  //~ }

  //~ cout << "Overall node count "<<nodecount<<endl;
  
  //~ int nc=0;
  //~ pts.resize(nodecount+1);

  
  //~ gmsh::model::getEntities(entities);
  //~ for(auto e : entities) {
    //~ cout<<" ---- \n"<<endl;
    //~ // Dimension and tag of the entity:
    //~ int dim = e.first, tag = e.second;


    //~ // Get the mesh nodes for the entity (dim, tag):
    //~ std::vector<std::size_t> nodeTags;
    //~ std::vector<double> nodeCoords, nodeParams;
    //~ gmsh::model::mesh::getNodes(nodeTags, nodeCoords, nodeParams, dim, tag);
    //~ //cout << "Node coords size "<<endl;
    //~ //cout << "Node Coords "<<nodeCoords[0]<<", " << nodeCoords[1]<<", "<<nodeCoords[2]<<endl;

    //~ // Get the mesh elements for the entity (dim, tag):
    //~ std::vector<int> elemTypes;
    //~ std::vector<std::vector<std::size_t> > elemTags, elemNodeTags;
    //~ gmsh::model::mesh::getElements(elemTypes, elemTags, elemNodeTags, dim, tag);

    
    //~ // * Number of mesh nodes and elements:
    //~ int numElem = 0;
    //~ cout << "Element tags size "<<elemTags.size()<<endl;
    //~ for(auto &tags : elemTags) numElem += tags.size();
    
      //~ // std::cout << " - Mesh has " << nodeTags.size() << " nodes and " << numElem
              //~ // << " elements\n";
      //~ // cout << "Node coords size "<<nodeCoords.size()<<endl; 
      
      //~ for (int n=0;n<nodeCoords.size()/3;n++){
        //~ for (int d=0;d<3;d++){
          //~ //cout << "Node "<<n<<": "<<nodeCoords[3*n+d]<<", "<<endl;
        //~ }
        //~ nodetagpos[nodeTags[n]]=nc;
        
        //~ // cout << "Node pos local"<<n << " and global "<<nc<<" has tag "<<nodeTags[n]<<endl;
          //~ //test[n][d]= nodeCoords[3*n+d];
          //~ //float coords[3];
          //~ std::array <float,3> coords;
          //~ for (int d=0;d<3;d++) coords[d] = nodeCoords[3*n+d];
          //~ // IF REAL POSITIONS
          //~ //pts.push_back(coords);

          //~ if (nodeTags[n]<nodecount)
            //~ pts[nodeTags[n]]=coords;
          //~ else
            //~ cout << "ERROR IN NODE "<<nodeTags[n]<<endl;

          //~ nc++;
        //~ //}
      //~ }
      //~ //cout << "Nodes inside nodeTags"<<endl;
      
      //~ for (auto n: nodeTags){
        //~ cout << n<<" ";
      //~ }   
      //~ cout << endl;

    //~ m_node_count = pts.size();
        
    //~ if (dim ==1){ 
      
      //~ cout << "Generating graphic mesh 1D "<<endl;
        //~ for(int ne=0;ne<elemNodeTags[0].size()/3;ne++)   { 
          //~ std::vector <int> conn; conn.resize(2);
          //~ //cout << "Local "  << elemNodeTags[0][2*ne] << ", "<<elemNodeTags[0][2*ne+1] <<endl;
          //~ //cout << "Global " << nodetagpos[elemNodeTags[0][3*ne]] <<", "<< nodetagpos[elemNodeTags[0][3*ne+1]] << endl;
          //~ for (int d=0;d<2;d++) {
            //~ conn[d] = elemNodeTags[0][2*ne+d];
            
            //~ //If defined with gmsh positions 
            //~ //conn[d] = nodetagpos[elemNodeTags[0][3*ne+d]] ;/*elemNodeTags[0][3*ne+d];

          //~ }
          //~ elnodes.push_back(conn);
        //~ }      
        
      //~ }else if (dim ==2){
      //~ for(auto &tags : elemTags){ 
        //~ //cout << "Element inside tags "<<endl;
        //~ //for (int t=0;t<tags.size();t++)
        //~ //  cout <<tags[t]<<" ";
        //~ //cout << endl;
        
        //~ //cout << endl<<"Element nodes size"<< elemNodeTags.size()<<", "<<elemNodeTags[0].size()<<endl;
        //~ //for(auto ne: elemNodeTags[0])   { 
          //~ //cout << ne ;//numElem += tags.size();          
        //~ //}
        //~ //cout << endl;
        
        //~ for(int ne=0;ne<elemNodeTags[0].size()/3;ne++)   { 
          //~ //std::array <int,3> conn;
          //~ std::vector<int> conn;
          //~ conn.resize(3);
          //~ //cout << "Local "  << elemNodeTags[0][3*ne] << ", "<<elemNodeTags[0][3*ne+1] << ", "<<elemNodeTags[0][3*ne+2] <<endl;
          //~ //cout << "Global " << nodetagpos[elemNodeTags[0][3*ne]] <<", "<< nodetagpos[elemNodeTags[0][3*ne+1]]<<", " << nodetagpos[elemNodeTags[0][3*ne+2]] <<endl;
          //~ for (int d=0;d<3;d++) {
            //~ conn[d] = elemNodeTags[0][3*ne+d];
            
            //~ //If defined with gmsh positions 
            //~ //conn[d] = nodetagpos[elemNodeTags[0][3*ne+d]] ;/*elemNodeTags[0][3*ne+d];*/

          //~ }
          //~ elnodes.push_back(conn);
        //~ }
      //~ }//elem tags
    //~ }// dim 2


    //~ // * List all types of elements making up the mesh of the entity:
    //~ for(auto elemType : elemTypes) {
      //~ std::string name;
      //~ int d, order, numv, numpv;
      //~ std::vector<double> param;
      //~ gmsh::model::mesh::getElementProperties(elemType, name, d, order, numv,
                                              //~ param, numpv);
      //~ //std::cout << " - Element type: " << name << ", order " << order << "\n";
      //~ //std::cout << "   with " << numv << " nodes in param coord: (";
      //~ //for(auto p : param) std::cout << p ;
      //~ //std::cout << ")\n";
    //~ }
    //~ cout << "elem tag size: "<<elemTags.size()<<", element nodetag size "<<elemNodeTags.size()<<endl; 
    //~ //for (auto enode : elemNodeTags[0]){
      //~ //cout << elemNodeTags[0][enode]<<endl;}
        
    
  //~ }//entities
  
  //~ cout << "Element Nodes size "<<elnodes.size()<<endl;
  
  //~ for (int p=0;p<pts.size();p++)
    //~ m_node.push_back(new Node(pts[p][0],pts[p][1],pts[p][2],p));

  //~ //TEMPLATIZE
  //~ for (int e=0;e<elnodes.size();e++){
    //~ int ne = elnodes[e].size();
    //~ if (ne==3){
      //~ //Element *elem = new ;
      //~ //vtkNew<vtkTriangle> tri;
      //~ /*
      //~ for (int nn=0;nn<elnodes[e].size();nn++) {
        //~ tri->GetPointIds()->SetId(nn, elnodes[e][nn]);
        //~ cout <<elnodes[e][nn]<<", ";
      //~ }
      //~ cout <<endl;
      //~ polys->InsertNextCell(tri);
      //~ */
    //~ } else if (ne ==2){
      //~ /*
      //~ vtkNew<vtkLine> tri;
      //~ for (int nn=0;nn<elnodes[e].size();nn++) {
        //~ tri->GetPointIds()->SetId(nn, elnodes[e][nn]);
        //~ cout <<elnodes[e][nn]<<", ";
      //~ }
      //~ cout <<endl;
      //~ //polys->InsertNextCell(tri);      
      //~ */
      //~ }
  //~ }
  
  
//~ }


void Mesh::genFromGmshModel() {
    // Limpiar estructuras existentes
    m_node.clear();
    m_elem.clear();

    // Obtener información del modelo
    cout << "Opening mesh "<<endl;
    std::string name;
    gmsh::model::getCurrent(name);
    std::cout << "Model " << name << " (" << gmsh::model::getDimension() << "D)\n";

    // Obtener todas las entidades del modelo
    std::vector<std::pair<int, int>> entities;
    gmsh::model::getEntities(entities);
    std::cout << "Entity count " << entities.size() << std::endl;

    // Contar dimensiones presentes
    int dim_count[] = {0, 0, 0, 0};
    for(auto e : entities) {
        int dim = e.first;
        if(dim >= 0 && dim < 4) dim_count[dim]++;
    }
    
    std::cout << "DIM COUNT: " << dim_count[0] << ", " << dim_count[1] << ", " 
              << dim_count[2] << ", " << dim_count[3] << std::endl;
    
    int max_dim = 0;
    for(int i = 0; i < 4; i++) {
        if(dim_count[i] > 0) max_dim = i;
    }
    std::cout << "Max dim " << max_dim << std::endl;

    // Mapa para relacionar tags de Gmsh con índices locales
    std::map<std::size_t, int> gmshToLocalIndex;
    std::vector<std::array<float, 3>> pts;
    std::vector<std::vector<int>> elnodes;

    // Primera pasada: recoger todos los puntos
    int nodeCount = 0;
    for(auto &e : entities) {
        int dim = e.first, tag = e.second;
        
        std::vector<std::size_t> nodeTags;
        std::vector<double> nodeCoords, nodeParams;
        gmsh::model::mesh::getNodes(nodeTags, nodeCoords, nodeParams, dim, tag);
        
        for(std::size_t i = 0; i < nodeTags.size(); i++) {
            if(gmshToLocalIndex.find(nodeTags[i]) == gmshToLocalIndex.end()) {
                std::array<float, 3> coord = {
                    static_cast<float>(nodeCoords[3 * i]),
                    static_cast<float>(nodeCoords[3 * i + 1]),
                    static_cast<float>(nodeCoords[3 * i + 2])
                };
                
                pts.push_back(coord);
                gmshToLocalIndex[nodeTags[i]] = nodeCount;
                nodeCount++;
            }
        }
    }

    std::cout << "Total nodes: " << nodeCount << std::endl;

    // Segunda pasada: procesar elementos
    int elementCount = 0;
    for(auto &e : entities) {
        int dim = e.first, tag = e.second;
        
        std::vector<int> elemTypes;
        std::vector<std::vector<std::size_t>> elemTags, elemNodeTags;
        gmsh::model::mesh::getElements(elemTypes, elemTags, elemNodeTags, dim, tag);

        for(std::size_t i = 0; i < elemTypes.size(); i++) {
            int elemType = elemTypes[i];
            std::string elemName;
            int elemDim, order, numNodes, numPrimaryNodes;
            std::vector<double> param;
            
            gmsh::model::mesh::getElementProperties(elemType, elemName, elemDim, order, 
                                                   numNodes, param, numPrimaryNodes);
            
            std::cout << "Processing element type: " << elemName << " with " << numNodes << " nodes" << std::endl;

            for(std::size_t j = 0; j < elemTags[i].size(); j++) {
                std::vector<int> connectivity;
                
                // Establecer la conectividad de la celda
                for(int k = 0; k < numNodes; k++) {
                    std::size_t nodeTag = elemNodeTags[i][j * numNodes + k];
                    if(gmshToLocalIndex.find(nodeTag) != gmshToLocalIndex.end()) {
                        int localIndex = gmshToLocalIndex[nodeTag];
                        connectivity.push_back(localIndex);
                    } else {
                        std::cerr << "Error: Node tag " << nodeTag << " not found in mapping!" << std::endl;
                    }
                }
                
                // Guardar conectividad
                elnodes.push_back(connectivity);
                elementCount++;
            }
        }
    }

    std::cout << "Total elements: " << elementCount << std::endl;

    // Crear nodos
    m_node_count = pts.size();
    for(std::size_t p = 0; p < pts.size(); p++) {
        m_node.push_back(new Node(pts[p][0], pts[p][1], pts[p][2], p));
    }

    // Crear elementos - INSERCIÓN SEGÚN EL TIPO DE ELEMENTO
    for(std::size_t e = 0; e < elnodes.size(); e++) {
        int numNodes = elnodes[e].size();
        std::vector<Node*> elementNodes;
        
        // Crear vector de nodos para el elemento
        for(int nodeIndex : elnodes[e]) {
            if(nodeIndex >= 0 && nodeIndex < m_node.size()) {
                elementNodes.push_back(m_node[nodeIndex]);
            } else {
                std::cerr << "Error: Invalid node index " << nodeIndex << " in element " << e << std::endl;
            }
        }

        //~ // Crear el elemento según el número de nodos
        //~ if(numNodes == 2) {
            //~ // Elemento 1D (Línea)
            //~ m_elem.push_back(new Line(elementNodes));
        //~ } else if(numNodes == 3) {
            //~ // Elemento 2D (Triángulo)
            //~ m_elem.push_back(new Tria(elementNodes));
        //~ } else if(numNodes == 4) {
            //~ // Podría ser Quad 2D o Tetraedro 3D
            //~ // Verificamos la dimensión máxima de la malla
            //~ if(max_dim == 2) {
                //~ m_elem.push_back(new Quad(elementNodes));
            //~ } else {
                //~ //m_elem.push_back(new Tetrahedron(elementNodes));
            //~ }
        //~ } else if(numNodes == 8) {
            //~ // Hexaedro 3D
            //~ //m_elem.push_back(new Hexahedron(elementNodes));
        //~ } else if(numNodes == 6) {
            //~ // Prisma triangular 3D
            //~ //m_elem.push_back(new Wedge(elementNodes));
        //~ } else if(numNodes == 5) {
            //~ // Pirámide 3D
            //~ //m_elem.push_back(new Pyramid(elementNodes));
        //~ } else {
            //~ std::cout << "Unsupported element with " << numNodes << " nodes" << std::endl;
            //~ // Crear elemento genérico
        
        
            m_elem.push_back(new Element(elementNodes));
        
        //}
    }

    // Establecer la dimensión de la malla
    m_dim = max_dim;
    
    m_elem_count  =m_elem.size();

    std::cout << "Mesh created successfully with " << m_node.size() 
              << " nodes and " << m_elem.size() << " elements" << std::endl;
    
    //~ // Si es una malla estructurada 2D, podemos organizar los elementos como en tu ejemplo
    //~ if(m_dim == 2 && isStructuredMesh(elnodes)) {
        //~ organizeStructuredMesh2D();
    //~ } else if(m_dim == 3 && isStructuredMesh(elnodes)) {
        //~ organizeStructuredMesh3D();
    //~ }
}

//~ // Función auxiliar para verificar si la malla es estructurada
//~ bool Mesh::isStructuredMesh(const std::vector<std::vector<int>>& elnodes) {
    //~ // Implementar lógica para detectar malla estructurada
    //~ // Por ahora, retornamos false para mallas no estructuradas
    //~ return false;
//~ }

//~ // Función para organizar malla estructurada 2D (similar a tu ejemplo)
//~ void Mesh::organizeStructuredMesh2D() {
    //~ // Implementación similar a tu ejemplo de Quad
    //~ // int nel[2] = {num_elems_x, num_elems_y};
    //~ // std::vector<Node*> n(4);
    
    //~ // for(int ey = 0; ey < nel[1]; ey++) {
    //~ //     for(int ex = 0; ex < nel[0]; ex++) {
    //~ //         n[0] = m_node[(nel[0]+1)*ey + ex];
    //~ //         n[1] = m_node[(nel[0]+1)*ey + ex+1];
    //~ //         n[2] = m_node[(nel[0]+1)*(ey+1) + ex+1];
    //~ //         n[3] = m_node[(nel[0]+1)*(ey+1) + ex];
    //~ //         m_elem.push_back(new Quad(n));
    //~ //     }
    //~ // }
//~ }

//~ // Función para organizar malla estructurada 3D (similar a tu ejemplo)
//~ void Mesh::organizeStructuredMesh3D() {
    //~ // Implementación similar a tu ejemplo de Hexahedron
    //~ // int nel[3] = {num_elems_x, num_elems_y, num_elems_z};
    //~ // std::vector<Node*> n(8);
    //~ // int nnodz = (nel[0]+1)*(nel[1]+1);
    
    //~ // for(int ez = 0; ez < nel[2]; ez++) {
    //~ //     for(int ey = 0; ey < nel[1]; ey++) {
    //~ //         for(int ex = 0; ex < nel[0]; ex++) {
    //~ //             int nb1 = nnodz*ez + (nel[0]+1)*ey + ex;
    //~ //             int nb2 = nnodz*ez + (nel[0]+1)*(ey+1) + ex;
                
    //~ //             n[0] = m_node[nb1];
    //~ //             n[1] = m_node[nb1+1];
    //~ //             n[2] = m_node[nb2+1];
    //~ //             n[3] = m_node[nb2];
    //~ //             n[4] = m_node[nb1 + nnodz];
    //~ //             n[5] = m_node[nb1 + nnodz + 1];
    //~ //             n[6] = m_node[nb2 + nnodz + 1];
    //~ //             n[7] = m_node[nb2 + nnodz];
                
    //~ //             m_elem.push_back(new Hexahedron(n));
    //~ //         }
    //~ //     }
    //~ // }
//~ }

#include <fstream>
#include <iomanip>
#include <sstream>

std::string formatLSDynaNumber(double value) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(13);
    
    // Para números negativos, el signo ocupa un espacio
    if (value < 0) {
        oss << std::setw(15) << value;
    } else {
        // Para números positivos, agregamos un espacio extra
        oss << " " << std::setw(15) << value;
    }
    return oss.str();
}

bool Mesh::exportToLSDYNA(const std::string& filename) {
    std::ofstream outfile(filename);
    if (!outfile.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << " for writing" << std::endl;
        return false;
    }

    // Escribir encabezado del archivo
    outfile << "*KEYWORD" << std::endl;
    outfile << "*TITLE" << std::endl;
    outfile << "LS-DYNA Export from Mesh Class" << std::endl;
    outfile << "Created by Mesh::exportToLSDYNA()" << std::endl;
    outfile << "$# END OF HEADER" << std::endl;
    

    // Escribir nodos
    outfile << "*NODE" << std::endl;
    outfile << "$#   nid               x               y               z      tc      rc" << std::endl;
    
    for (size_t i = 0; i < m_node.size(); i++) {
        Node* node = m_node[i];
        //const std::array<float, 3>& pos = node->getPos();
        Vector3f pos = node->getPos();
        outfile << std::setw(8) << i + 1   // ID del nodo (1-based)
                << std::scientific << std::setprecision(10)
                << formatLSDynaNumber(pos.x)  
                << formatLSDynaNumber(pos.y)  
                << formatLSDynaNumber(pos.z)  
                << std::setw(6) << 0       // tc (default 0)
                << std::setw(6) << 0       // rc (default 0)
                << std::endl;
    }
    outfile << "$# END OF NODES" << std::endl;
    

    // Escribir elementos
    int shellElementCount = 0;
    int solidElementCount = 0;

    for (size_t i = 0; i < m_elem.size(); i++) {
        Element* elem = m_elem[i];
        int numNodes = elem->getNodeCount();

        // Determinar el tipo de elemento LS-DYNA
        int elemType = 0;
        //~ std::string elemName;

        //~ if (numNodes == 2) {
            //~ // Elemento barra (BEAM)
            //~ elemType = 1;
            //~ elemName = "BEAM";
        //~ } else if (numNodes == 3) {
            //~ // Shell triangular (SHELL)
            //~ elemType = 2;
            //~ elemName = "SHELL";
            //~ shellElementCount++;
        //~ } else if (numNodes == 4) {
            //~ if (m_dim == 2) {
                //~ // Shell cuadrangular (SHELL)
                //~ elemType = 2;
                //~ elemName = "SHELL";
                //~ shellElementCount++;
            //~ } else {
                //~ // Tetraedro (SOLID)
                //~ elemType = 4;
                //~ elemName = "SOLID";
                //~ solidElementCount++;
            //~ }
        //~ } else if (numNodes == 8) {
            //~ // Hexaedro (SOLID)
            //~ elemType = 1; // SOLID tipo 1 en LS-DYNA
            //~ elemName = "SOLID";
            //~ solidElementCount++;
        //~ }

        // Escribir encabezado de sección de elementos si es necesario
        if (i == 0) {
            cout << "ELEM NAME" <<endl;
            if (m_dim == 2)
            //outfile << "*ELEMENT_" << elemName << std::endl;
            outfile << "*ELEMENT_SHELL" << std::endl;
            else
            //outfile << "*ELEMENT_" << elemName << std::endl;
            outfile << "*ELEMENT_SOLID" << std::endl;
            outfile << "$#   eid     pid      n1      n2      n3      n4      n5      n6      n7      n8" << std::endl;
        }

        
        bool out = false;
        if (m_dim == 2){
          if (numNodes== 3 || numNodes == 4)
            out = true;
          } else if (m_dim == 3){
            
            if (numNodes==4)
            out = true;
        }
        if (out){
          // Escribir elemento
          outfile << std::setw(8) << i + 1   // ID del elemento (1-based)
                  << std::setw(8) << 1 ;     // PID (Part ID) - default 1          
          // Escribir conectividad de nodos
          for (int j = 0; j < std::min(numNodes, 8); j++) {
              int nodeId = elem->getNodeId(j)+1;
              outfile << std::setw(8) << nodeId ;
          }

          // Completar con ceros si tiene menos de 8 nodos
          for (int j = numNodes; j < 8; j++) {
              outfile << std::setw(8) << 0 ;
          }

        outfile << std::endl;
        
      }// if out
    }// Elements
    outfile << "$# END OF ELEMENTS" << std::endl;
    

    // Escribir definición de PART
    outfile << "*PART" << std::endl;
    outfile << "$#     pid       secid       mid     eosid      hgid      grav    adpopt      tmid" << std::endl;
    outfile << std::setw(8) << 1   // PID
            << std::setw(8) << 1   // SECID
            << std::setw(8) << 1   // MID (Material ID)
            << std::setw(8) << 0   // EOSID
            << std::setw(8) << 0   // HGID
            << std::setw(8) << 0   // GRAV
            << std::setw(8) << 0   // ADPOPT
            << std::setw(8) << 0   // TMID
            << std::endl;
    outfile << "$# END OF PART" << std::endl;
    

    // Escribir definición de SECTION según el tipo de elementos
    if (shellElementCount > 0) {
        outfile << "*SECTION_SHELL" << std::endl;
        outfile << "$#   secid    elform      shrf       nip     propt   qr/irid     icomp     setyp" << std::endl;
        outfile << std::setw(8) << 1   // SECID
                << std::setw(8) << 2   // ELFORM (2: Belytschko-Tsay shell)
                << std::setw(8) << 1.0  // SHRF (shear factor)
                << std::setw(8) << 2   // NIP (integration points)
                << std::setw(8) << 1.0  // PROPT
                << std::setw(8) << 0   // QR/IRID
                << std::setw(8) << 0   // ICOMP
                << std::setw(8) << 1   // SETYP (shell type)
                << std::endl;
        outfile << "$# END OF SHELL SECTION" << std::endl;
        
    }

    if (solidElementCount > 0) {
        outfile << "*SECTION_SOLID" << std::endl;
        outfile << "$#   secid    elform       aet    unused    unused    unused    unused    unused" << std::endl;
        outfile << std::setw(8) << 2   // SECID (diferente al de shells)
                << std::setw(8) << 1   // ELFORM (1: constant stress solid)
                << std::setw(8) << 0   // AET
                << std::setw(8) << 0   // unused
                << std::setw(8) << 0   // unused
                << std::setw(8) << 0   // unused
                << std::setw(8) << 0   // unused
                << std::setw(8) << 0   // unused
                << std::endl;
        outfile << "$# END OF SOLID SECTION" << std::endl;
        
    }

    // Escribir definición de MATERIAL básico
    outfile << "*MAT_ELASTIC" << std::endl;
    outfile << "$#     mid        ro         e        pr        da        db         k" << std::endl;
    outfile << std::setw(8) << 1   // MID
            << std::scientific << std::setprecision(6)
            << std::setw(12) << 7850.0   // RO (densidad)
            << std::setw(12) << 2.1e11   // E (módulo elástico)
            << std::setw(12) << 0.3      // PR (coef. Poisson)
            << std::setw(12) << 0.0      // DA
            << std::setw(12) << 0.0      // DB
            << std::setw(12) << 0.0      // K
            << std::endl;
    outfile << "$# END OF MATERIAL" << std::endl;
    

    // Escribir condiciones de contorno básicas (ejemplo)
    outfile << "*BOUNDARY_SPC_SET" << std::endl;
    outfile << "$#      id       cid      dofx      dofy      dofz     dofrx     dofry     dofrz" << std::endl;
    outfile << std::setw(8) << 1   // ID
            << std::setw(8) << 0   // CID
            << std::setw(8) << 1   // DOFX (fijo)
            << std::setw(8) << 1   // DOFY (fijo)
            << std::setw(8) << 1   // DOFZ (fijo)
            << std::setw(8) << 0   // DOFRX (libre)
            << std::setw(8) << 0   // DOFRY (libre)
            << std::setw(8) << 0   // DOFRZ (libre)
            << std::endl;
    outfile << "$# END OF BOUNDARY CONDITIONS" << std::endl;
    

    // Escribir terminador del archivo
    outfile << "*END" << std::endl;
    
    outfile.close();

    std::cout << "LS-DYNA export completed successfully!" << std::endl;
    std::cout << "File: " << filename << std::endl;
    std::cout << "Nodes: " << m_node.size() << std::endl;
    std::cout << "Elements: " << m_elem.size() << std::endl;
    std::cout << "Shell elements: " << shellElementCount << std::endl;
    std::cout << "Solid elements: " << solidElementCount << std::endl;

    return true;
}


#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <string>
#include <iostream>

// Suponiendo que tienes estas clases definidas

//~ class Element {
//~ public:
    //~ int getNodeCount() const { return 0; }
    //~ int getNodeId(int) const { return 0; }
    //~ std::string getType() const { return ""; }
    //~ // ... otras funciones
//~ };

//~ class Mesh {
//~ public:
    //~ std::vector<Node*> m_node;
    //~ std::vector<Element*> m_elem;
    //~ int m_dim;
    
    //~ bool exportToNASTRAN(const std::string& filename);
//~ };

// Función auxiliar para formatear números con ancho fijo de 8 caracteres
std::string formatNastranNumber(double value) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(5);
    
    // Para números negativos, el signo ocupa un espacio
    if (value < 0) {
        oss << std::setw(7) << value;
    } else {
        // Para números positivos, agregamos un espacio extra
        oss << " " << std::setw(7) << value;
    }
    return oss.str();
}

//~ // Función auxiliar para formatear números con ancho fijo de 8 caracteres
//~ std::string formatNastranNumber(double value) {
    //~ std::ostringstream oss;
    //~ oss << std::fixed << std::scientific << std::setprecision(1);
    
    //~ // Para números negativos, el signo ocupa un espacio
    //~ if (value < 0) {
        //~ oss << std::setw(7) << value;
    //~ } else {
        //~ // Para números positivos, agregamos un espacio extra
        //~ oss << " " << std::setw(7) << value;
    //~ }
    //~ return oss.str();
//~ }

// Función para formatear en notación científica sin punto decimal
std::string formatScientific(double value) {
    std::ostringstream oss;
    
    if (value == 0.0) {
        oss << std::setw(8) << "0";
        return oss.str();
    }
    
    int exponent = static_cast<int>(std::floor(std::log10(std::abs(value))));
    double mantissa = value * std::pow(10, -exponent);
    
    // Ajustar mantissa para que esté entre 1 y 10 (sin punto decimal)
    int int_mantissa = static_cast<int>(std::round(mantissa));
    
    // Formatear: mantissaE+exponente (ejemplo: 1234E-05)
    oss << std::setw(4) << int_mantissa 
        << "E" 
        << std::setw(3) << std::setfill('0') 
        << (exponent >= 0 ? "+" : "-")
        << std::setw(2) << std::abs(exponent);
    
    return oss.str();
}

//~ std::string formatNastranNumber(double value) {
    //~ std::ostringstream oss;
    //~ oss << std::fixed << std::setprecision(6) 
        //~ << std::internal << std::setw(8) << value;
    //~ return oss.str();
//~ }

bool Mesh::exportToNASTRAN(const std::string& filename) {
    std::ofstream outfile(filename);
    if (!outfile.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << " for writing" << std::endl;
        return false;
    }

    // Escribir encabezado del archivo NASTRAN
    outfile << "NASTRAN bulk data file created by Mesh::exportToNASTRAN()" << std::endl;
    outfile << "BEGIN BULK" << std::endl;

       
    // Escribir nodos (GRID)
    outfile << "$ Nodes" << std::endl;
    for (size_t i = 0; i < m_node.size(); i++) {
        Node* node = m_node[i];
        auto pos = node->getPos();
        
        outfile << "GRID    " 
            << std::setw(8) << i + 1  // ID del nodo (1-based)
            << std::setw(8) << 0       // CP
            << formatNastranNumber(pos[0])
            << formatNastranNumber(pos[1])
            << formatNastranNumber(pos[2]) 
            << std::endl;
    }
    outfile << "$ End of nodes" << std::endl;

    // Contadores para diferentes tipos de elementos
    int cquad4Count = 0;
    int ctri3Count = 0;
    int cbarCount = 0;
    int cbeamCount = 0;
    int ctria3Count = 0;
    int ctetraCount = 0;
    int chexaCount = 0;

    // Escribir elementos
    outfile << "$ Elements" << std::endl;
    for (size_t i = 0; i < m_elem.size(); i++) {
        Element* elem = m_elem[i];
        int numNodes = elem->getNodeCount();
        //std::string elemType = elem->getType();

//        if (elemType == "BAR" || numNodes == 2) {

        //DIMENSION SHOULD BE 1 TO WRITE THIS 
        if ( numNodes == 2 && m_dim == 1) {
            // Elemento barra CBAR
            outfile << "CBAR    " << std::setw(8) << i + 1  // EID
                    << std::setw(8) << 1          // PID
                    << std::setw(8) << elem->getNodeId(0) + 1  // GA
                    << std::setw(8) << elem->getNodeId(1) + 1  // GB
                    //<< ",,0.0,1.0,0.0" // Vector de orientación 
                    << std::endl;     
            cbarCount++;
        }
//        else if (elemType == "BEAM" || (numNodes == 2 && elemType == "BEAM")) {
            // // Elemento viga CBEAM
            // outfile << "CBEAM," << std::setw(8) << i + 1  // EID
                    // << std::setw(8) << 2           // PID
                    // << std::setw(8) << elem->getNodeId(0) + 1  // GA
                    // << std::setw(8) << elem->getNodeId(1) + 1  // GB
                    // << ",,0.0,1.0,0.0" << std::endl;     // Vector de orientación
            // cbeamCount++;
        // }
        else if (numNodes == 3 && m_dim == 2) {
            // Elemento triangular 2D CTRIA3
            outfile << "CTRIA3  " << std::setw(8) << i + 1  // EID
                    << std::setw(8) << 3           // PID
                    << std::setw(8) << elem->getNodeId(0) + 1
                    << std::setw(8) << elem->getNodeId(1) + 1
                    << std::setw(8) << elem->getNodeId(2) + 1
                    << std::endl;
            ctri3Count++;
        }
        else if (numNodes == 4 && m_dim == 2) {
            // Elemento cuadrilátero 2D CQUAD4
            //~ outfile << "CQUAD4," << std::setw(8) << i + 1  // EID
                    //~ << std::setw(8) << 4           // PID
                    //~ << std::setw(8) << elem->getNodeId(0) + 1
                    //~ << std::setw(8) << elem->getNodeId(1) + 1
                    //~ << std::setw(8) << elem->getNodeId(2) + 1
                    //~ << std::setw(8) << elem->getNodeId(3) + 1
                    //~ << std::endl;
            //~ cquad4Count++;
            outfile << "CTRIA3  " << std::setw(8) << 2*i + 1  // EID
                    << std::setw(8) << 3           // PID
                    << std::setw(8) << elem->getNodeId(0) + 1
                    << std::setw(8) << elem->getNodeId(1) + 1
                    << std::setw(8) << elem->getNodeId(2) + 1
                    << std::endl;
            ctri3Count++;
            outfile << "CTRIA3  " << std::setw(8) << 2*i + 2  // EID
                    << std::setw(8) << 3           // PID
                    << std::setw(8) << elem->getNodeId(0) + 1
                    << std::setw(8) << elem->getNodeId(2) + 1
                    << std::setw(8) << elem->getNodeId(3) + 1
                    << std::endl;
            ctri3Count++;
        }
        else if (numNodes == 4 && m_dim == 3) {
            // Elemento tetraédrico CTETRA
            outfile << "CTETRA  " << std::setw(8) << i + 1  // EID
                    << std::setw(8) << 5           // PID
                    << std::setw(8) << elem->getNodeId(0) + 1
                    << std::setw(8) << elem->getNodeId(1) + 1
                    << std::setw(8) << elem->getNodeId(2) + 1
                    << std::setw(8) << elem->getNodeId(3) + 1
                    << std::endl;
            ctetraCount++;
        }
        else if (numNodes == 8 && m_dim == 3) {
            // Elemento hexaédrico CHEXA
            outfile << "CHEXA   " << std::setw(8) << i + 1  // EID
                    << std::setw(8) << 6           // PID
                    << std::setw(8) << elem->getNodeId(0) + 1
                    << std::setw(8) << elem->getNodeId(1) + 1
                    << std::setw(8) << elem->getNodeId(2) + 1
                    << std::setw(8) << elem->getNodeId(3) + 1
                    << std::setw(8) << elem->getNodeId(4) + 1
                    << std::setw(8) << elem->getNodeId(5) + 1
                    << std::endl;
            // Continuación para nodos 7 y 8 si es necesario
            chexaCount++;
        }
    }
    outfile << "$ End of elements" << std::endl;

    // Escribir propiedades de elementos (PBAR, PBEAM, PSHELL, etc.)
    outfile << "$ Properties" << std::endl;
    
    if (cbarCount > 0) {
        outfile << "PBAR    " << std::setw(8) << 1  // PID
                << std::setw(8) << 1      // MID
                << ",1.0,0.1,0.01,0.01"         // A, I1, I2, J
                << std::endl;
    }
    
    if (cbeamCount > 0) {
        outfile << "PBEAM   " << std::setw(8) << 2  // PID
                << std::setw(8) << 1       // MID
                << ",1.0,0.1,0.01,0.01"          // A, I1, I2, J
                << std::endl;
    }
    
    if (ctri3Count > 0 || cquad4Count > 0) {
        outfile << "PSHELL  " << std::setw(8) << 3  // PID
                << std::setw(8) << 1        // MID
                << ",0.1"                         // thickness
                << std::endl;
    }

    // Escribir materiales
    //~ outfile << "$ Materials" << std::endl;
    //~ outfile << "MAT1," << std::setw(8) << 1        // MID
            //~ << ",2.1E11,,0.3,7850.0"              // E, G, NU, RHO
            //~ << std::endl;

    //~ // Condiciones de contorno (SPC)
    //~ outfile << "$ Boundary conditions" << std::endl;
    //~ outfile << "SPC,1,123456,1" << std::endl;  // SPC set 1, todos los DOFs, node 1

    // Finalizar archivo
    outfile << "ENDDATA" << std::endl;
    
    outfile.close();

    std::cout << "NASTRAN export completed successfully!" << std::endl;
    std::cout << "File: " << filename << std::endl;
    std::cout << "Nodes: " << m_node.size() << std::endl;
    std::cout << "Elements: " << m_elem.size() << std::endl;
    std::cout << "CBAR elements: " << cbarCount << std::endl;
    std::cout << "CBEAM elements: " << cbeamCount << std::endl;
    std::cout << "CTRIA3 elements: " << ctri3Count << std::endl;
    std::cout << "CQUAD4 elements: " << cquad4Count << std::endl;
    std::cout << "CTETRA elements: " << ctetraCount << std::endl;
    std::cout << "CHEXA elements: " << chexaCount << std::endl;

    return true;
}
