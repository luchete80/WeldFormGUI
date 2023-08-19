#include <assert.h>

#include "mymesh.h"
#include "ogldev_engine_common.h"

using namespace std;

#define POSITION_LOCATION 0
#define TEX_COORD_LOCATION 1
#define NORMAL_LOCATION 2


#include <iostream>
using namespace std;

myMesh::myMesh()
{
    m_VAO = 0;
    ZERO_MEM(m_Buffers);
}


myMesh::~myMesh()
{
    Clear();
}


void myMesh::Clear()
{
    for (unsigned int i = 0 ; i < m_Textures.size() ; i++) {
        SAFE_DELETE(m_Textures[i]);
    }

    if (m_Buffers[0] != 0) {
        glDeleteBuffers(ARRAY_SIZE_IN_ELEMENTS(m_Buffers), m_Buffers);
    }
       
    if (m_VAO != 0) {
        glDeleteVertexArrays(1, &m_VAO);
        m_VAO = 0;
    }
}


bool myMesh::LoadMesh(const string& Filename)
{
    // // Release the previously loaded mesh (if it exists)
    // Clear();
 
    // // Create the VAO
    // glGenVertexArrays(1, &m_VAO);   
    // glBindVertexArray(m_VAO);
    
    // // Create the buffers for the vertices attributes
    // glGenBuffers(ARRAY_SIZE_IN_ELEMENTS(m_Buffers), m_Buffers);

    // bool Ret = false;
    // Assimp::Importer Importer;

    // const aiScene* pScene = Importer.ReadFile(Filename.c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_FindDegenerates);
    
    // if (pScene) {
        // Ret = InitFromScene(pScene, Filename);
    // }
    // else {
        // printf("Error parsing '%s': '%s'\n", Filename.c_str(), Importer.GetErrorString());
    // }

    // // Make sure the VAO is not changed from the outside
    // glBindVertexArray(0);	

    // return Ret;
}

//////////////////////
////// LUCIANO
////// THIS IS SIMILAR TO OGLDEV LOAD MESH FROM ASSIMP BUT IS WITH A VERTEX LIST
bool myMesh::LoadMesh(    vector<Vector3f> Positions,
    vector<Vector3f> Normals,
    vector<Vector2f> TexCoords,
    vector<unsigned int> Indices,
	string &texfname) {

    Clear();
 
    // Create the VAO
    glGenVertexArrays(1, &m_VAO);   
    glBindVertexArray(m_VAO);
    
    // Create the buffers for the vertices attributes
    glGenBuffers(ARRAY_SIZE_IN_ELEMENTS(m_Buffers), m_Buffers);
	
	
    m_Entries.resize(1);
    m_Textures.resize(1);

    // unsigned int NumVertices = 0;
    // unsigned int NumIndices = 0;
    
    // Count the number of vertices and indices
    //for (unsigned int i = 0 ; i < m_Entries.size() ; i++) {
	m_Entries[0].MaterialIndex = 0;        
	m_Entries[0].NumIndices = Indices.size();
	m_Entries[0].BaseVertex = 0;
	m_Entries[0].BaseIndex = 0;
	
		
	cout << "Loading material"<<endl;
	if (!InitMaterial(texfname))
		return false;
	cout << "Material created"<<endl;
	GenAndBindBuffers(Positions, Normals, TexCoords, Indices);

    // Make sure the VAO is not changed from the outside
    glBindVertexArray(0);
	
	return true;
}

// bool myMesh::InitFromScene(const aiScene* pScene, const string& Filename)
// {  
    // m_Entries.resize(pScene->mNumMeshes);
    // cout << "Num meshes: "<<pScene->mNumMeshes<<endl;
    // m_Textures.resize(pScene->mNumMaterials);

    // vector<Vector3f> Positions;
    // vector<Vector3f> Normals;
    // vector<Vector2f> TexCoords;
    // vector<unsigned int> Indices;

    // unsigned int NumVertices = 0;
    // unsigned int NumIndices = 0;
    
    // // Count the number of vertices and indices
    // for (unsigned int i = 0 ; i < m_Entries.size() ; i++) {
        // m_Entries[i].MaterialIndex = pScene->mMeshes[i]->mMaterialIndex;        
        // m_Entries[i].NumIndices = pScene->mMeshes[i]->mNumFaces * 3;
        // m_Entries[i].BaseVertex = NumVertices;
        // m_Entries[i].BaseIndex = NumIndices;
        
        // NumVertices += pScene->mMeshes[i]->mNumVertices;
        // NumIndices  += m_Entries[i].NumIndices;
    // }
    
    // // Reserve space in the vectors for the vertex attributes and indices
    // Positions.reserve(NumVertices);
    // Normals.reserve(NumVertices);
    // TexCoords.reserve(NumVertices);
    // Indices.reserve(NumIndices);

    // // Initialize the meshes in the scene one by one
    // for (unsigned int i = 0 ; i < m_Entries.size() ; i++) {
        // const aiMesh* paiMesh = pScene->mMeshes[i];
        // InitMesh(paiMesh, Positions, Normals, TexCoords, Indices);
    // }

    // if (!InitMaterials(pScene, Filename)) {
        // return false;
    // }

	// return GenAndBindBuffers(Positions,Normals,TexCoords,Indices);
// }

bool myMesh::GenAndBindBuffers(
    vector<Vector3f> Positions,
    vector<Vector3f> Normals,
    vector<Vector2f> TexCoords,
    vector<unsigned int> Indices){
	// Generate and populate the buffers with vertex attributes and the indices
    glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[POS_VB]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Positions[0]) * Positions.size(), &Positions[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(POSITION_LOCATION);
    glVertexAttribPointer(POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0);    

    glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[TEXCOORD_VB]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(TexCoords[0]) * TexCoords.size(), &TexCoords[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(TEX_COORD_LOCATION);
    glVertexAttribPointer(TEX_COORD_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[NORMAL_VB]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Normals[0]) * Normals.size(), &Normals[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(NORMAL_LOCATION);
    glVertexAttribPointer(NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Buffers[INDEX_BUFFER]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices[0]) * Indices.size(), &Indices[0], GL_STATIC_DRAW);

    return GLCheckError();
	
}

// void myMesh::InitMesh(const aiMesh* paiMesh,
                    // vector<Vector3f>& Positions,
                    // vector<Vector3f>& Normals,
                    // vector<Vector2f>& TexCoords,
                    // vector<unsigned int>& Indices)
// {    
    // const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

    // // Populate the vertex attribute vectors
    // for (unsigned int i = 0 ; i < paiMesh->mNumVertices ; i++) {
        // const aiVector3D* pPos      = &(paiMesh->mVertices[i]);
        // const aiVector3D* pNormal   = &(paiMesh->mNormals[i]);
        // const aiVector3D* pTexCoord = paiMesh->HasTextureCoords(0) ? &(paiMesh->mTextureCoords[0][i]) : &Zero3D;

        // Positions.push_back(Vector3f(pPos->x, pPos->y, pPos->z));
        // Normals.push_back(Vector3f(pNormal->x, pNormal->y, pNormal->z));
        // TexCoords.push_back(Vector2f(pTexCoord->x, pTexCoord->y));
    // }

    // // Populate the index buffer
    // for (unsigned int i = 0 ; i < paiMesh->mNumFaces ; i++) {
        // const aiFace& Face = paiMesh->mFaces[i];
        // assert(Face.mNumIndices == 3);
        // Indices.push_back(Face.mIndices[0]);
        // Indices.push_back(Face.mIndices[1]);
        // Indices.push_back(Face.mIndices[2]);
    // }
// }

// bool myMesh::InitMaterials(const aiScene* pScene, const string& Filename)
// {
    // // Extract the directory part from the file name
    // string::size_type SlashIndex = Filename.find_last_of("/");
    // string Dir;

    // if (SlashIndex == string::npos) {
        // Dir = ".";
    // }
    // else if (SlashIndex == 0) {
        // Dir = "/";
    // }
    // else {
        // Dir = Filename.substr(0, SlashIndex);
    // }

    // bool Ret = true;

    // // Initialize the materials
    // for (unsigned int i = 0 ; i < pScene->mNumMaterials ; i++) {
        // const aiMaterial* pMaterial = pScene->mMaterials[i];

        // m_Textures[i] = NULL;

        // if (pMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
            // aiString Path;

            // if (pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &Path, NULL, NULL, NULL, NULL, NULL) == AI_SUCCESS) {
                // string p(Path.data);
                
                // if (p.substr(0, 2) == ".\\") {                    
                    // p = p.substr(2, p.size() - 2);
                // }
                               
                // string FullPath = Dir + "/" + p;
                    
                // m_Textures[i] = new Texture(GL_TEXTURE_2D, FullPath.c_str());

                // if (!m_Textures[i]->Load()) {
                    // printf("Error loading texture '%s'\n", FullPath.c_str());
                    // delete m_Textures[i];
                    // m_Textures[i] = NULL;
                    // Ret = false;
                // }
                // else {
                    // printf("Loaded texture '%s'\n", FullPath.c_str());
                // }
            // }
        // }
    // }

    // return Ret;
// }
// TODO; REPLACE ALL THIS IN BOTH InitMaterial function with another function
// TODO: ADD A SHADER TO THIS
bool myMesh::InitMaterial(const string& Filename) {
    // Extract the directory part from the file name
    string::size_type SlashIndex = Filename.find_last_of("/");
    string Dir;

    if (SlashIndex == string::npos) {
        Dir = ".";
    } else if (SlashIndex == 0) {
        Dir = "/";
    } else {
        Dir = Filename.substr(0, SlashIndex);
    }

    bool Ret = true;
	m_Textures[0] = NULL;
	//string FullPath = Dir + "/" + p;                 
	m_Textures[0] = new Texture(GL_TEXTURE_2D, Filename.c_str());

	if (!m_Textures[0]->Load()) {
		printf("Error loading texture '%s'\n", Filename.c_str());
		delete m_Textures[0];
		m_Textures[0] = NULL;
		Ret = false;
	} else {
		printf("Loaded texture '%s'\n", Filename.c_str());
	}
    return Ret;
}


void myMesh::Render() {
   
    glBindVertexArray(m_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[POS_VB]);
    glEnableVertexAttribArray(POSITION_LOCATION);
    glVertexAttribPointer(POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0);    

    glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[TEXCOORD_VB]);
    glEnableVertexAttribArray(TEX_COORD_LOCATION);
    glVertexAttribPointer(TEX_COORD_LOCATION, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[NORMAL_VB]);
    glEnableVertexAttribArray(NORMAL_LOCATION);
    glVertexAttribPointer(NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_Buffers[INDEX_BUFFER]);
    
    for (unsigned int i = 0 ; i < m_Entries.size() ; i++) {
        const unsigned int MaterialIndex = m_Entries[i].MaterialIndex;

        assert(MaterialIndex < m_Textures.size());
        
        if (m_Textures[MaterialIndex]) {
            m_Textures[MaterialIndex]->Bind(COLOR_TEXTURE_UNIT);
        }

        // glDrawElementsBaseVertex(GL_TRIANGLES, 
                         // m_Entries[i].NumIndices, 
                         // GL_UNSIGNED_INT, 
                         // (void*)(sizeof(unsigned int) * m_Entries[i].BaseIndex), 
                         // m_Entries[i].BaseVertex);

        glDrawElementsBaseVertex(GL_LINES, 
                         m_Entries[i].NumIndices, 
                         GL_UNSIGNED_INT, 
                         (void*)(sizeof(unsigned int) * m_Entries[i].BaseIndex), 
                         m_Entries[i].BaseVertex);

        // glDrawElements(GL_TRIANGLES, 
                         // m_Entries[i].NumIndices, 
                         // GL_UNSIGNED_INT, 
                         // (void*)(sizeof(unsigned int) * m_Entries[i].BaseIndex) 
                         // //,m_Entries[i].BaseVertex
                         // );
    }

    glBindBuffer(GL_ARRAY_BUFFER,0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
    // Make sure the VAO is not changed from the outside    
    glBindVertexArray(0);
}

void myMesh::Render(unsigned int NumInstances, const Matrix4f* WVPMats, const Matrix4f* WorldMats)
{        
    glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[WVP_MAT_VB]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Matrix4f) * NumInstances, WVPMats, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[WORLD_MAT_VB]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Matrix4f) * NumInstances, WorldMats, GL_DYNAMIC_DRAW);

    glBindVertexArray(m_VAO);
    
    for (unsigned int i = 0 ; i < m_Entries.size() ; i++) {
        const unsigned int MaterialIndex = m_Entries[i].MaterialIndex;

        assert(MaterialIndex < m_Textures.size());
        
        if (m_Textures[MaterialIndex]) {
            m_Textures[MaterialIndex]->Bind(GL_TEXTURE0);
        }

		glDrawElementsInstancedBaseVertex(GL_TRIANGLES, 
                                          m_Entries[i].NumIndices, 
                                          GL_UNSIGNED_INT, 
                                          (void*)(sizeof(unsigned int) * m_Entries[i].BaseIndex), 
                                          NumInstances,
                                          m_Entries[i].BaseVertex);
    }

    // Make sure the VAO is not changed from the outside    
    glBindVertexArray(0);
}

