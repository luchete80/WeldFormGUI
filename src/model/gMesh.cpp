#include "gMesh.h"
#include "Mesh.h"
#include "common/math/math.h" //float Vector2 and 3f
#include <iostream>
#include "Node.h"

gMesh::gMesh(Mesh* msh){
  m_msh = msh;
    	//// VERTICES 
	//// 2 3
	//// 0 1
	/////////
			

	Vector2f atex[4] ={	Vector2f(0.0f, 0.0f),
                      Vector2f(1.0f, 0.0f),
                      Vector2f(0.0f, 1.0f),
						Vector2f(1.0f, 1.0f)};

	unsigned int aind[] = { 0, 1, 2,
							1, 3, 2};
							   
	int vcount    = sizeof(sphere_low_pos)/(3*sizeof(float));
  int indcount  = sizeof(sphere_low_ind)/sizeof(unsigned int);
	// int vcount    = sizeof(vertices)/(3*sizeof(float));
  // int indcount  = sizeof(indices)/sizeof(unsigned int);
  cout << "Vertex count " << vcount << endl;
  cout << "Index  count " << indcount << endl;
  
	vector <Vector3f> vpos(vcount), vnorm(vcount);
	vector <Vector2f> vtex(vcount);
	vector <unsigned int > vind(indcount); //2 triangles
  
	for (int i=0;i<vcount;i++){
    //Vector3f vert(sphere_low_pos[3*i],sphere_low_pos[3*i+1],sphere_low_pos[3*i+2]);
		vpos[i]	= msh->getNode(i)->getPos();
    //Vector3f vn(sphere_low_norm[3*i],sphere_low_norm[3*i+1],sphere_low_norm[3*i+2]); //IF NORM IS READED FROM FILE
		//vnorm[i]=vn;
		//vtex[i]	=atex[i];
	}
  for (int i=0;i<indcount;i++){
    vind[i] = sphere_low_ind[i]-1;  //FROM OBJ FILE FORMAT, WHICH BEGINS AT ONE
  }
  
  int elemcount = indcount/3; //ATTENTION: THIS ASSUMES ALL IS CONVERTED TO TRIA
  std::vector<Vector3f> vnprom(vcount);

  for (int e=0;e<elemcount;e++){
    int i = vind[3*e]; //Element First node
    int j = vind[3*e+1];
    int k = vind[3*e+2];
    Vector3f r0(sphere_low_pos[3*i],sphere_low_pos[3*i+1],sphere_low_pos[3*i+2]);
    Vector3f r1(sphere_low_pos[3*j],sphere_low_pos[3*j+1],sphere_low_pos[3*j+2]);
    Vector3f r2(sphere_low_pos[3*k],sphere_low_pos[3*k+1],sphere_low_pos[3*k+2]);
    Vector3f v1 = r1-r0;
    Vector3f v2 = r2-r0;
    Vector3f vnn = (v1.Cross(v2)).Normalize();
    for (int l=0;l<3;l++) {vnprom[vind[3*e+l]]+=vnn;}
  }
  
  cout << "Normals"<<endl;
  for (int i=0;i<vcount;i++){
    vnprom[i].Normalize();
    cout << vnprom[i].x<< ", "<<vnprom[i].y<<", "<<vnprom[i].z<<endl;
  }
}