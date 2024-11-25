#include "Mesh.h"
#include "Node.h"

// #include <glm/gtc/matrix_transform.hpp>
void SPHMesh::addBoxLength(Vector3f V, Vector3f L, double r){
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
  cout << "SPH Mesh created. Element count: "<< nel[0]<<", "<<nel[1]<<", "<<nel[2]<<endl;



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
            //cout << "node " << p <<"X: "<<Xp[0]<<"Y: "<<Xp.y<<"Z: "<<Xp.z<<endl;
            m_node.push_back(new Node(Xp.x,Xp.y,Xp.z,p));
            p++;
            Xp.x += 2.0 * r;
          }
          Xp.y += 2.0 * r;
        }// 
         Xp.z += 2 * r;     
      }//for k
      
    }

    
		int ex, ey, ez;
		std::vector <Node*> n;
    if (m_dim == 2) {

       //m_elem.push_back(new Quad(n));
				
    } else { //dim: 3


		}//if dim 

  
} // ADD BOX LENGTH
