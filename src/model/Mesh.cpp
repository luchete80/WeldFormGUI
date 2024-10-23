#include "Mesh.h"
#include "Node.h"

#include <iostream>
#include <map>

#include <gmsh.h>

using namespace std;

/*
void initValues(  std::vector <Node*>    m_node, //LOCATED ON MODEL SPACE!!!!
                    std::vector <int>      elnod){
  

}
*/

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
  
// #include <glm/gtc/matrix_transform.hpp>
void Mesh::addBoxLength(Vector3f V, Vector3f L, double r){
    Vector3f Xp;
    
    int p, nnodz;
    int nel[3];
    int m_dim = 2;
    
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
            cout << "largest ind "<<nb2 + nnodz + 1<<endl;
            for (int i=0;i<8;i++)
              cout << n[i]->getId()<<", ";
            cout <<endl;
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


void Mesh::genFromGmshModel() {

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
  cout << "Entity count "<<entities.size()<<endl;


  std::vector <std::array<float,3>> pts; 

  //std::vector <std::array<int,3>> elnodes; 
  std::vector <std::vector <int> > elnodes; 
  std::map< int,int > nodetagpos;
  int nodecount =0;

  int dim_count[] = {0,0,0,0};
  for(auto e : entities) {
    int dim = e.first, tag = e.second;
    dim_count[dim]++;    
  }
  
  cout << "DIM COUNT "<<endl;
  cout << dim_count[0]<<", "<< dim_count[1]<<", "<< dim_count[2]<<", "<< dim_count[3]<<", "<<endl;
  int max_dim=0;
  for (int i=0;i<4;i++)
    if (dim_count[i]>0) 
      max_dim=i;
  cout << "Max dim "<<max_dim<<endl;
  
  for(auto e : entities) {

    int dim = e.first, tag = e.second;

    // Get the mesh nodes for the entity (dim, tag):
    std::vector<std::size_t> nodeTags;
    std::vector<double> nodeCoords, nodeParams;
    gmsh::model::mesh::getNodes(nodeTags, nodeCoords, nodeParams, dim, tag);


  }
  cout << "Overall node count "<<nodecount<<endl;
  
  int nc=0;
  pts.resize(nodecount+1);

  
  gmsh::model::getEntities(entities);
  for(auto e : entities) {
    cout<<" ---- \n"<<endl;
    // Dimension and tag of the entity:
    int dim = e.first, tag = e.second;


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
          cout << "Local "  << elemNodeTags[0][3*ne] << ", "<<elemNodeTags[0][3*ne+1] << ", "<<elemNodeTags[0][3*ne+2] <<endl;
          cout << "Global " << nodetagpos[elemNodeTags[0][3*ne]] <<", "<< nodetagpos[elemNodeTags[0][3*ne+1]]<<", " << nodetagpos[elemNodeTags[0][3*ne+2]] <<endl;
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
  
  for (int p=0;p<pts.size();p++)
    m_node.push_back(new Node(pts[p][0],pts[p][1],pts[p][2],p));

  //TEMPLATIZE
  for (int e=0;e<elnodes.size();e++){
    int ne = elnodes[e].size();
    if (ne==3){
      //Element *elem = new ;
      //vtkNew<vtkTriangle> tri;
      /*
      for (int nn=0;nn<elnodes[e].size();nn++) {
        tri->GetPointIds()->SetId(nn, elnodes[e][nn]);
        cout <<elnodes[e][nn]<<", ";
      }
      cout <<endl;
      polys->InsertNextCell(tri);
      */
    } else if (ne ==2){
      /*
      vtkNew<vtkLine> tri;
      for (int nn=0;nn<elnodes[e].size();nn++) {
        tri->GetPointIds()->SetId(nn, elnodes[e][nn]);
        cout <<elnodes[e][nn]<<", ";
      }
      cout <<endl;
      //polys->InsertNextCell(tri);      
      */
      }
  }
  
  
}

