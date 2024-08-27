#ifndef SHAPES_H
#define SHAPES_H

#include "mymesh.h"

//  B     D
//A    B

//  F     H
//E    G



const float vertices[] = {                  //Cube array
/*------------------Triangular EGC*/
 -0.5f, -0.5f, -0.5f, 1.0f,0.0f,0.0f,//E
 0.5f, -0.5f, -0.5f,  1.0f,0.0f,0.0f,//G
 0.5f,  0.5f, -0.5f,  1.0f,0.0f,0.0f,//C
  
 /*------------------Triangular CAE*/
 0.5f,  0.5f, -0.5f,  1.0f,0.0f,0.0f,//C
 -0.5f,  0.5f, -0.5f,  1.0f,0.0f,0.0f,//A
 -0.5f, -0.5f, -0.5f,  1.0f,0.0f,0.0f,//E
  
  /*------------------Triangular FHD*/
 -0.5f, -0.5f,  0.5f,  0.0f,1.0f,0.0f,//F
 0.5f, -0.5f,  0.5f,  0.0f,1.0f,0.0f,//H
 0.5f,  0.5f,  0.5f,  0.0f,1.0f,0.0f,//D
  
  /*------------------Triangular DBF*/
 0.5f,  0.5f,  0.5f,  0.0f,1.0f,0.0f,//D
 -0.5f,  0.5f,  0.5f,  0.0f,1.0f,0.0f,//B
 -0.5f, -0.5f,  0.5f,  0.0f,1.0f,0.0f,//F
  
  /*------------------Triangle BAE----------------------*/
 -0.5f,  0.5f,  0.5f,  0.0f,0.0f,1.0f,//B
 -0.5f,  0.5f, -0.5f,  0.0f,0.0f,1.0f,//A
 -0.5f, -0.5f, -0.5f,  0.0f,0.0f,1.0f,//E
  
  /*------------------Triangular EFB*/
 -0.5f, -0.5f, -0.5f,  0.0f,0.0f,1.0f,//E
 -0.5f, -0.5f,  0.5f,  0.0f,0.0f,1.0f,//F
 -0.5f,  0.5f,  0.5f,  0.0f,0.0f,1.0f,//B
  
  /*------------------Triangular DCG*/
 0.5f,  0.5f,  0.5f,  0.5f,0.0f,0.0f,//D
 0.5f,  0.5f, -0.5f,  0.5f,0.0f,0.0f,//C
 0.5f, -0.5f, -0.5f,  0.5f,0.0f,0.0f,//G
  
  /*------------------Triangular GHD*/
 0.5f, -0.5f, -0.5f,  0.5f,0.0f,0.0f,//G
 0.5f, -0.5f,  0.5f,  0.5f,0.0f,0.0f,//H
 0.5f,  0.5f,  0.5f,  0.5f,0.0f,0.0f,//D
  
  /*------------------Triangular EGH*/
 -0.5f, -0.5f, -0.5f,  0.0f,0.5f,0.0f,//E
 0.5f, -0.5f, -0.5f,  0.0f,0.5f,0.0f,//G
 0.5f, -0.5f,  0.5f,  0.0f,0.5f,0.0f,//H
  
  /*------------------Triangle HFE*/
 0.5f, -0.5f,  0.5f,  0.0f,0.5f,0.0f,//H
 -0.5f, -0.5f,  0.5f,  0.0f,0.5f,0.0f,//F
 -0.5f, -0.5f, -0.5f,  0.0f,0.5f,0.0f,//E
  
  /*------------------Triangle ACD*/
 -0.5f,  0.5f, -0.5f,  0.0f,0.0f,0.5f,//A
 0.5f,  0.5f, -0.5f,  0.0f,0.0f,0.5f,//C
 0.5f,  0.5f,  0.5f,  0.0f,0.0f,0.5f,//D
  
  /*------------------Triangular DBA*/
 0.5f,  0.5f,  0.5f,  0.0f,0.0f,0.5f,//D
 -0.5f,  0.5f,  0.5f,  0.0f,0.0f,0.5f,//B
 -0.5f,  0.5f, -0.5f,  0.0f,0.0f,0.5f//A
};

//void generateQuadXZ();

//LUCIANO: THIS IS RETURNING A MESH LIKE IN PROTOBALL
bool GenerateQuadMesh(myMesh* mesh);

bool RenderLine(const Matrix4f* WVPMats, const Matrix4f* WorldMats);

#endif