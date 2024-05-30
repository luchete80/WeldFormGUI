#ifndef _Renderer_
#define _Renderer_

#include <map>
#include <vector>
#include <glad/glad.h>

// #include <assimp/Importer.hpp>      // C++ importer interface
// #include <assimp/scene.h>       // Output data structure
// #include <assimp/postprocess.h> // Post processing flags

#include "ogldev_util.h"
//#include "Math.h" //In "Common" Dir
#include "ogldev_texture.h"
#include "ogldev_pipeline.h"

#include <string>
#include "Domain.h"

#include "model/Mesh.h"

///////////////////////////
//////// LA DIFERENCIA ES QUE ESTA MALLA NO SE CARGA DE UN ARCHIVO
//////////////////////////
#define INVALID_MATERIAL 0xFFFFFFFF
   
#define INDEX_BUFFER  0    
#define POS_VB        1
#define NORMAL_VB     2
#define TEXCOORD_VB   3    
#define WVP_MAT_VB    4
#define WORLD_MAT_VB  5
#define IDX_WIREFRAME 6    

struct myVertex
{
    Vector3f m_pos;
    Vector2f m_tex;
    Vector3f m_normal;

    myVertex() {}

    myVertex(const Vector3f& pos, const Vector2f& tex, const Vector3f& normal)
    {
        m_pos    = pos;
        m_tex    = tex;
        m_normal = normal;
    }
};

struct BasicMeshEntry {
    BasicMeshEntry()
        {
            NumIndices = 0;
            BaseVertex = 0;
            BaseIndex = 0;
            MaterialIndex = INVALID_MATERIAL;
        }
        
        unsigned int NumIndices;
        unsigned int NumIndicesWF;
		unsigned int BaseVertex;
        unsigned int BaseIndex;
        unsigned int MaterialIndex;
    };

class Renderer
{
public:
    void addMesh   (Mesh *); //adds fem mesh
    void addDomain (SPH::Domain *); //adds fem mesh
    Renderer();

    ~Renderer();

    bool LoadMesh(const std::string& Filename);
	bool LoadMesh(    vector<Vector3f> Positions,
					vector<Vector3f> Normals,
					vector<Vector2f> TexCoords,
					vector<unsigned int> Indices,
          vector<unsigned int> wf_Indices,
					string &texfname);

    void Render();
	
    void Render(unsigned int NumInstances, const Matrix4f* WVPMats, const Matrix4f* WorldMats);
    
    Orientation& GetOrientation() { return m_orientation; }



private:
  GLuint m_VAO;
  GLuint m_Buffers[7];

    //bool InitFromScene(const aiScene* pScene, const std::string& Filename);
    // void InitMesh(const aiMesh* paiMesh,
                  // std::vector<Vector3f>& Positions,
                  // std::vector<Vector3f>& Normals,
                  // std::vector<Vector2f>& TexCoords,
                  // std::vector<unsigned int>& Indices);

  //bool InitMaterials(const aiScene* pScene, const std::string& Filename);
	bool InitMaterial(const string& Filename);
    void Clear();
	bool GenAndBindBuffers(vector<Vector3f> Positions,vector<Vector3f> Normals, vector<Vector2f> TexCoords,
                          vector<unsigned int> Indices,
                          vector<unsigned int> wireframe_Indices); //NEW


    
    std::vector<BasicMeshEntry> m_Entries;
    std::vector<Texture*> m_Textures;
    Orientation m_orientation;
};

#endif