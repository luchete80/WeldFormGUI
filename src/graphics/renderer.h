#ifndef _DEBUG_RENDERER_H_
#define _DEBUG_RENDERER_H_

#include <glad/glad.h>
#include <vector>
#include "math.h"
#include "ogldev_math_3d.h"

//#include "camera_system.h"

//LIKE IN BULLET3
struct GraphicsInstance
{
	unsigned int m_cube_vao;
	unsigned int m_index_vbo;
	unsigned int m_textureIndex;
	int m_numIndices;
	int m_numVertices;
};

//
//Build by LUCIANO
//IS LIKE BULLET RENDERER

 //TODO: PUT LESS ARGUMENTS IN DRAWLINE FUNCTIONS
// drawLines(const float* positions, const float color[4], 
                                        // int numPoints,   
                                        // int pointStrideInBytes, 
                                        // const unsigned int* indices, int numIndices, 
                                        // float lineWidthIn)
//// TODO: ASOCIATE WITH SCENENODE!
                       
class Camera;
                       
class Renderer {

public:
  Renderer();
  //void AddCamera(Camera *camera){m_Camera = camera;} //Put as a Scene Node??
 
 
  void drawLines();
  void drawLine(const float* positions, int pointStrideInBytes);
  
  //TODO: MAKE TWO FUNCTIONS! ONCE WITH float* as opengl and another with Vector3f
  //void Renderer::drawLines(const float* positions, const float color[4], 
  void drawLines(/*Vector3f *Vertices*/
                  const float* positions, const float color[4], 
                                        int numPoints,   
                                        const unsigned int* indices, int numIndices, 
                                        float lineWidthIn, Matrix4f *WVP = NULL);


  //Bullet3 style Instancing renderer
  // void drawLines(const float* positions, const float color[4], 
                                        // int numPoints,   
                                        // int pointStrideInBytes, 
                                        // const unsigned int* indices, int numIndices, 
                                        // float lineWidthIn,
                                        // Matrix4f *WVP = NULL);
                                        
  void RenderDebug();
  
 
private:
  unsigned int VBO_Lines; //SAME FOR ALL LINES
  unsigned int VAO_Lines;
  unsigned int IBO_Lines;
  
  // Camera*           m_Camera;
  // CameraSystem*     m_CameraSystem;
  
  GLuint shaderProgram;
   
  std::vector <struct GraphicsInstance*> m_graphicsInstances;
                                          
  //Formerly this was in game class
  //ATTENTION! THIS IS ALSO IN CAMERA SYSTEM
  PersProjInfo 								m_persProjInfo;
  
  GLuint gWVPLocation;    //For transform things from camera matrix, for instance
  GLuint colour;    //For transform things from camera matrix, for instance
  unsigned int WINDOW_WIDTH;
  unsigned int WINDOW_HEIGHT;
  
  bool test_static;  //
  
};

// TODO: UNIFY WITH THE EXISTENT (OGLDEV)
void read_shader_src(const char *fname, std::vector<char> &buffer);
unsigned int  load_and_compile_shader(const char *fname, GLenum shaderType);
unsigned int   create_program(const char *path_vert_shader, const char *path_frag_shader);


#endif