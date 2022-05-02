#include "shapes.h"
#define PITCH_LENGTH 50.
#include <iostream>

#include "mymesh.h"
#include "ogldev_basic_mesh.h"

using namespace std;

// ORIGINAL FROM OPENGLDEV
//Is like generating vertex buffer
// void generateQuad(p1,p2) {
    // Vertex Vertices[4] = { Vertex(Vector3f(-1.0f, -1.0f, 0.5773f), Vector2f(0.0f, 0.0f)),
                           // Vertex(Vector3f(0.0f, -1.0f, -1.15475f), Vector2f(0.5f, 0.0f)),
                           // Vertex(Vector3f(1.0f, -1.0f, 0.5773f),  Vector2f(1.0f, 0.0f)),
                           // Vertex(Vector3f(0.0f, 1.0f, 0.0f),      Vector2f(0.5f, 1.0f)) };
    
 	// glGenBuffers(1, &VBO);
	// glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);
// }

///////////////////////////////
//  GENERATE A MESH IN XZ PLANE BUT RETURNS TO A MESH CLASS
///////////////////////////////

bool GenerateQuadMesh (myMesh *mesh){
	vector<Vector3f> vpos(4), vnorm(4);
	vector<Vector2f> vtex(4);
	vector <unsigned int > vind(6);
	//// VERTICES 
	//// 2 3
	//// 0 1
	/////////
	Vector3f apos[4] ={	
						Vector3f(-PITCH_LENGTH/2., 0.0f,-PITCH_LENGTH/2.),
						Vector3f(-PITCH_LENGTH/2., 0.0f, PITCH_LENGTH/2.),
						Vector3f( PITCH_LENGTH/2., 0.0f,-PITCH_LENGTH/2.),
						Vector3f( PITCH_LENGTH/2., 0.0f, PITCH_LENGTH/2.)};
						
	Vector3f norm=Vector3f(0.,1.,0.);	

	Vector2f atex[4] ={	Vector2f(0.0f, 0.0f),
						 Vector2f(1.0f, 0.0f),
						Vector2f(0.0f, 1.0f),
						Vector2f(1.0f, 1.0f)};

	unsigned int aind[] = { 0, 1, 2,
							1, 3, 2};
							   
	
	for (int i=0;i<4;i++){
		vpos[i]	=apos[i];
		vnorm[i]=norm;
		vtex[i]	=atex[i];
	}
	
	for (int i=0;i<6;i++) vind[i] = aind[i];
	string file = "field.png";
	
	if (!mesh->LoadMesh(vpos, vnorm, vtex,vind,file)){
		std::cout<<"Mesh load failed"<<endl;
		printf("Mesh load failed\n");
		return false;        			
		
	}
  
}

GLuint linesVertexArrayObject = 0;

//OGLDEV
//Mesh Style

// bool RenderLine(const Matrix4f* WVPMats, const Matrix4f* WorldMats){
    // glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[WVP_MAT_VB]);
    // glBufferData(GL_ARRAY_BUFFER, sizeof(Matrix4f) * NumInstances, WVPMats, GL_DYNAMIC_DRAW);

    // glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[WORLD_MAT_VB]);
    // glBufferData(GL_ARRAY_BUFFER, sizeof(Matrix4f) * NumInstances, WorldMats, GL_DYNAMIC_DRAW);

    // glBindVertexArray(m_VAO);
    
    // for (unsigned int i = 0 ; i < m_Entries.size() ; i++) {
        // const unsigned int MaterialIndex = m_Entries[i].MaterialIndex;

        // assert(MaterialIndex < m_Textures.size());
        
        // if (m_Textures[MaterialIndex]) {
            // m_Textures[MaterialIndex]->Bind(GL_TEXTURE0);
        // }

		// glDrawElementsInstancedBaseVertex(GL_TRIANGLES, 
                                          // m_Entries[i].NumIndices, 
                                          // GL_UNSIGNED_INT, 
                                          // (void*)(sizeof(unsigned int) * m_Entries[i].BaseIndex), 
                                          // NumInstances,
                                          // m_Entries[i].BaseVertex);
    // }

    // // Make sure the VAO is not changed from the outside    
    // glBindVertexArray(0); 

// }

/// EXAMPLE MESH RENDERER
//
  
// // #define INDEX_BUFFER 0    
// // #define POS_VB       1
// // #define NORMAL_VB    2
// // #define TEXCOORD_VB  3    
// // #define WVP_MAT_VB   4
// // #define WORLD_MAT_VB 5
// IN BASIC MESH
    // // GLuint m_VAO;
    // // GLuint m_Buffers[6];

    // // struct BasicMeshEntry {
        // // BasicMeshEntry()
        // // {
            // // NumIndices = 0;
            // // BaseVertex = 0;
            // // BaseIndex = 0;
            // // MaterialIndex = INVALID_MATERIAL;
        // // }
        
        // // unsigned int NumIndices;
		// // unsigned int BaseVertex;
        // // unsigned int BaseIndex;
        // // unsigned int MaterialIndex;
    // // };
    
    
// void myMesh::Render(unsigned int NumInstances, const Matrix4f* WVPMats, const Matrix4f* WorldMats)
// {        
    // glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[WVP_MAT_VB]);
    // glBufferData(GL_ARRAY_BUFFER, sizeof(Matrix4f) * NumInstances, WVPMats, GL_DYNAMIC_DRAW);

    // glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[WORLD_MAT_VB]);
    // glBufferData(GL_ARRAY_BUFFER, sizeof(Matrix4f) * NumInstances, WorldMats, GL_DYNAMIC_DRAW);

    // glBindVertexArray(m_VAO);
    
    // for (unsigned int i = 0 ; i < m_Entries.size() ; i++) {
        // const unsigned int MaterialIndex = m_Entries[i].MaterialIndex;

        // assert(MaterialIndex < m_Textures.size());
        
        // if (m_Textures[MaterialIndex]) {
            // m_Textures[MaterialIndex]->Bind(GL_TEXTURE0);
        // }

		// glDrawElementsInstancedBaseVertex(GL_TRIANGLES, 
                                          // m_Entries[i].NumIndices, 
                                          // GL_UNSIGNED_INT, 
                                          // (void*)(sizeof(unsigned int) * m_Entries[i].BaseIndex), 
                                          // NumInstances,
                                          // m_Entries[i].BaseVertex);
    // }

    // // Make sure the VAO is not changed from the outside    
    // glBindVertexArray(0);
// }

//Line in openGL



/// HOW TO WORK WITH GLInstancingRenderer::drawLines 
//()IN bullet3/examples/OpenGLWindow/SimpleOpenGL3App

			// b3Assert(glGetError() == GL_NO_ERROR);
			// b3Vector3 from = b3MakeVector3(0, 0, 0);
			// from[sideAxis] = float(-gridSize);
			// from[upAxis] = upOffset;
			// from[forwardAxis] = float(i);
			// b3Vector3 to = b3MakeVector3(0, 0, 0);
			// to[sideAxis] = float(gridSize);
			// to[upAxis] = upOffset;
			// to[forwardAxis] = float(i);
			// vertices.push_back(from);
			// indices.push_back(lineIndex++);
			// vertices.push_back(to);
			// indices.push_back(lineIndex++);
			// // m_instancingRenderer->drawLine(from,to,gridColor);
		// }
	// }

	// m_instancingRenderer->drawLines(&vertices[0].x,
									// gridColor,
									// vertices.size(), sizeof(b3Vector3), &indices[0], indices.size(), 1);
                  

// THIS IS LIKE IN 2D 
////////////// IN bullet3/examples/OpenGLWindow
// void GLInstancingRenderer::drawLines(const float* positions, const float color[4], int numPoints, int pointStrideInBytes, const unsigned int* indices, int numIndices, float lineWidthIn)
// {
	// glActiveTexture(GL_TEXTURE0);
	// glBindTexture(GL_TEXTURE_2D, 0);

	// float lineWidth = lineWidthIn;
	// b3Clamp(lineWidth, (float)lineWidthRange[0], (float)lineWidthRange[1]);
	// glLineWidth(lineWidth);

	// glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// glActiveTexture(GL_TEXTURE0);
	// glBindTexture(GL_TEXTURE_2D, 0);

	// b3Assert(glGetError() == GL_NO_ERROR);
	// glUseProgram(linesShader);
	// glUniformMatrix4fv(lines_ProjectionMatrix, 1, false, &m_data->m_projectionMatrix[0]);
	// glUniformMatrix4fv(lines_ModelViewMatrix, 1, false, &m_data->m_viewMatrix[0]);
	// glUniform4f(lines_colour, color[0], color[1], color[2], color[3]);

	// //	glPointSize(pointDrawSize);
	// glBindVertexArray(linesVertexArrayObject);

	// b3Assert(glGetError() == GL_NO_ERROR);
	// glBindBuffer(GL_ARRAY_BUFFER, linesVertexBufferObject);

	// {
		// glBufferData(GL_ARRAY_BUFFER, numPoints * pointStrideInBytes, 0, GL_DYNAMIC_DRAW);

		// glBufferSubData(GL_ARRAY_BUFFER, 0, numPoints * pointStrideInBytes, positions);
		// b3Assert(glGetError() == GL_NO_ERROR);
		// glBindBuffer(GL_ARRAY_BUFFER, 0);
		// glBindBuffer(GL_ARRAY_BUFFER, linesVertexBufferObject);
		// glEnableVertexAttribArray(0);

		// b3Assert(glGetError() == GL_NO_ERROR);
		// int numFloats = 3;
		// glVertexAttribPointer(0, numFloats, GL_FLOAT, GL_FALSE, pointStrideInBytes, 0);
		// b3Assert(glGetError() == GL_NO_ERROR);

		// glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, linesIndexVbo);
		// int indexBufferSizeInBytes = numIndices * sizeof(int);

		// glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufferSizeInBytes, NULL, GL_DYNAMIC_DRAW);
		// glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indexBufferSizeInBytes, indices);

		// glDrawElements(GL_LINES, numIndices, GL_UNSIGNED_INT, 0);
	// }

	// glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	// glBindBuffer(GL_ARRAY_BUFFER, 0);
	// //	for (int i=0;i<numIndices;i++)
	// //		printf("indicec[i]=%d]\n",indices[i]);
	// b3Assert(glGetError() == GL_NO_ERROR);
	// glBindVertexArray(0);
	// b3Assert(glGetError() == GL_NO_ERROR);
	// glPointSize(1);
	// b3Assert(glGetError() == GL_NO_ERROR);
	// glUseProgram(0);
// }