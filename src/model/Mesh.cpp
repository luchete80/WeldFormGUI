#include "Mesh.h"
#include "Node.h"

Mesh::addBoxLength(const double &L[3], const double &V[3], double r){
    Node Xp;
    
    int p, nnodz;
    int nel[3];
    int m_dim = 2;
    
    if (L[2] > 0.0) m_dim = 3;
    
    
    nel[0] = (int)(L[0]/(2.0*r));
    nel[1] = (int)(L[1]/(2.0*r));
    cout << "Nel x: "<<nel[0]<<", y "<<nel[1]<<endl;
    
    // m_gp_count = 1;
    // if (m_dim == 2){
      // nel[2] = 1;
      // m_nodxelem = 4;
      // if (!red_int) m_gp_count = 4;
    // } else {
      // nel[2] = (int)(L.z/(2.0*r));
      // m_nodxelem = 8;
      // if (!red_int) m_gp_count = 8; 
    // }
    

    Xp[2] = V[2] ;
    

    // // write (*,*) "Creating Mesh ...", "Elements ", neL.y, ", ",neL.z
  int nc = (nel[0] +1) * (nel[1]+1) * (nel[2]+1);
  int ne = nel[0]*nel[1]*nel[2];
  cout << "Mesh created. Element count: "<< nel[0]<<", "<<nel[1]<<", "<<nel[2]<<endl;
  
  // //thisAllocateNodes((nel[0] +1) * (nel[1]+1) * (nel[2]+1));
    // // print *, "Element count in XYZ: ", nel(:)
    // // write (*,*) "Box Node count ", node_count

	// this->SetDimension(nc,ne);	 //AFTER CREATING DOMAIN
  // cout << "Mesh generated. Node count: " << nc<<". Element count: "<<ne<<endl;
  // cout << "Dimension is: "<<m_dim<<endl;
  // //SPH::Domain	dom;
	// //vector_t *x =  (vector_t *)malloc(dom.Particles.size());
	// vector_t *x_H =  new vector_t [m_node_count];


	// //int size = dom.Particles.size() * sizeof(vector_t);
	// cout << "Copying to device..."<<endl;
    
    // cout << "Box Particle Count is " << m_node_count <<endl;
    // p = 0;
    // for (int k = 0; k < (nel[2] +1);k++) {
      // Xp.y = V.y;
      // for (int j = 0; j < (nel[1] +1);j++){
        // Xp[0] = V[0];
        // for (int i = 0; i < (nel[0] +1);i++){
					// //m_node.push_back(new Node(Xp));
					// x_H[p] = Xp;
          // //nod%x(p,:) = Xp(:);
          // cout << "node " << p <<"X: "<<Xp[0]<<"Y: "<<Xp.y<<"Z: "<<Xp.z<<endl;
          // p++;
          // Xp[0] = Xp[0] + 2.0 * r;
        // }
        // Xp.y = Xp.y + 2.0 * r;
      // }// 
      // Xp.z = Xp.z + 2 * r;

    // //cout <<"m_node size"<<m_node.size()<<endl;
    // } 
		// memcpy_t(this->x, x_H, sizeof(vector_t) * m_node_count);    

    // // !! ALLOCATE ELEMENTS
    // // !! DIMENSION = 2
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

		// elnod_h       = new int [m_elem_count * m_nodxelem]; //Flattened

     // int *nodel_count_h  = new int [m_node_count];
     // int *nodel_offset_h = new int [m_node_count];
    
		// int ex, ey, ez;
		// std::vector <int> n;
    // if (m_dim == 2) {
			// n.resize(4);
      // int ei = 0;
      // for (int ey = 0; ey < nel[1];ey++){
        // for (int ex = 0; ex < nel[0];ex++){
        // int iv[4];
        // elnod_h[ei  ] = (nel[0]+1)*ey + ex;        elnod_h[ei+1] = (nel[0]+1)*ey + ex+1;
        // elnod_h[ei+2] = (nel[0]+1)*(ey+1) + ex+1;  elnod_h[ei+3] = (nel[0]+1)*(ey+1) + ex;
			
				 // for (int i=0;i<m_nodxelem;i++)cout << elnod_h[ei+i]<<", ";
					// cout << "Nel x : "<<nel[0]<<endl;
					// cout << "nodes "<<endl;
					// ei += m_nodxelem;
					 // }
      // } 
    // } else { //dim: 3
      // int ei = 0; //ELEMENT INTERNAL NODE (GLOBAL INDEX)
      // int nnodz = (nel[0]+1)*(nel[1]+1);
      // for (int ez = 0; ez < nel[2];ez++)
      // for (int ey = 0; ey < nel[1];ey++){
        // for (int ex = 0; ex < nel[0];ex++){
          
          // int iv[8];
          // int nb1 = nnodz*ez + (nel[0]+1)*ey + ex;
          // int nb2 = nnodz*ez + (nel[0]+1)*(ey+1) + ex;
          
          // elnod_h[ei  ] = nb1;                      nodel_count_h[nb1  ] ++;          
          // elnod_h[ei+1] = nb1+1;                    nodel_count_h[nb1+1] ++;
          // elnod_h[ei+2] = nb2+1;                    nodel_count_h[nb2+1] ++;
          // elnod_h[ei+3] = nb2;                      nodel_count_h[nb2  ] ++;
          
          // elnod_h[ei+4] = nb1 + nnodz*(ez+1);       nodel_count_h[nb1 + nnodz*(ez+1)    ]++;   
          // elnod_h[ei+5] = nb1 + nnodz*(ez+1) + 1;   nodel_count_h[nb1 + nnodz*(ez+1) + 1]++;  
          // elnod_h[ei+6] = nb2 + nnodz*(ez+1) + 1;   nodel_count_h[nb2 + nnodz*(ez+1) + 1]++;  
          // elnod_h[ei+7] = nb2 + nnodz*(ez+1);       nodel_count_h[nb2 + nnodz*(ez+1)    ]++;  
          
          // for (int i=0;i<8;i++)
            // cout << elnod_h[ei + i]<<", ";
          // cout <<endl;

            // cout << "Nel x : "<<nel[0]<<endl;
           // cout << "nodes "<<endl;
           
           // for (int i=0;i<m_nodxelem;i++)cout << elnod_h[ei+i]<<", ";
           // ei += m_nodxelem;

					 // }
      // } 

		// }//if dim 

    // //cudaMalloc((void **)&m_elnod, m_elem_count * m_nodxelem * sizeof (int));	
    // malloc_t(m_elnod, unsigned int, m_elem_count * m_nodxelem);
		// memcpy_t(this->m_elnod, elnod_h, sizeof(int) * m_elem_count * m_nodxelem);    
    
    // //cudaMalloc(&m_jacob,m_elem_count * sizeof(Matrix ));
    // malloc_t(m_jacob, Matrix, m_elem_count );
    
    // //////////////////// ELEMENT SHARED BY NODES (FOR PARALLEL NODAL MODE ASSEMBLY) ///////////////////////////////
    // int nodel_tot = 0;
    // for (int n=0;n<m_node_count;n++){
      // nodel_offset_h[n] = nodel_tot;
      // nodel_tot        += nodel_count_h[n];
      // cout << "Node "<< n << " Shared elements: "<<nodel_count_h[n]<<endl;

    // }
    // cout << "Size of Nodal shared Elements vector "<< nodel_tot<<endl;
		// int *nodel_h       = new int [nodel_tot];          //ASSUMED EACH NODE SHARES 8 ELEMENT
    // int *nodel_loc_h   = new int [nodel_tot];          //ASSUMED EACH NODE SHARES 8 ELEMENT    
    
    // for (int n=0;n<m_node_count;n++)  nodel_count_h[n] = 0;    
    // for (int e=0;e<m_elem_count;e++){
      // int offset = m_nodxelem * e;
      // for (int ne=0;ne<m_nodxelem;ne++){
        // int n = elnod_h[offset+ne];
        
        // nodel_h     [nodel_offset_h[n] + nodel_count_h[n]] = e;
        // nodel_loc_h [nodel_offset_h[n] + nodel_count_h[n]] = ne;
        
        // nodel_count_h[n]++;
      // }//nod x elem 
    // }

    // // cudaMalloc((void **)&m_nodel,     nodel_tot * sizeof (int));
    // // cudaMalloc((void **)&m_nodel_loc, nodel_tot * sizeof (int));
    
    // malloc_t (m_nodel,        int,nodel_tot);
    // malloc_t (m_nodel_loc,    int,nodel_tot);
    
    // malloc_t (m_nodel_offset, int, m_node_count);
    // malloc_t (m_nodel_count,  int, m_node_count);
    
    // //THIS IS ONLY FOR COMPLETENESS IN CASE OF CPU, SHOULD BE BETTER TO WRITE ON FINALL ARRAY
		// memcpy_t(this->m_nodel,         nodel_h,        sizeof(int) * nodel_tot); 
		// memcpy_t(this->m_nodel_loc,     nodel_loc_h,    sizeof(int) * nodel_tot);
		 
		// memcpy_t(this->m_nodel_offset,  nodel_offset_h, sizeof(int) * m_node_count);  //OFFSET FOR PREVIOUS ARRAYS
		// memcpy_t(this->m_nodel_count,    nodel_count_h, sizeof(int) * m_node_count);  //OFFSET FOR PREVIOUS ARRAYS
		    
    // // ///// TESTING
    // for (int n=0;n<m_node_count;n++){
      // cout << "M node offset:"<<nodel_offset_h[n];
      // cout << "Node  "<< n << " Elements"<<endl;
      // for (int ne=0;ne<nodel_count_h[n];ne++) cout << nodel_h[nodel_offset_h[n]+ne]<<", ";
      // cout << endl;
      // cout << "Node  "<< n << " Elements Internal Node"<<endl;
      // for (int ne=0;ne<nodel_count_h[n];ne++) cout << nodel_loc_h[nodel_offset_h[n]+ne]<<", ";
      // cout << endl;
    // }
    
  

		
		// delete [] /*elnod_h, */nodel_count_h, nodel_h, nodel_loc_h,nodel_offset_h;
  
}
