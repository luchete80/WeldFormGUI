///////////////////////////////////////////////
////////////// RENDERER ///////////////////////
///// 0 - LINES: This gonna be a large file (20220406)
#include "Renderer.h"
#include "Math.h"

#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
using namespace std;

Renderer::Renderer(){
  //test_static = true;
  test_static = false;
  if (!test_static) {
    glGenVertexArrays(1, &VAO_Lines);
    glBindVertexArray(VAO_Lines);

   
   
    // //If statically  
    // //glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices), Indices, GL_DYNAMIC_DRAW);
    
  // Create a Vector Buffer Object that will store the vertices on video memory
    //GLuint vbo; //In tutorial this was static since all elements did not chage
    glGenBuffers(1, &VBO_Lines);
 
  unsigned int Indices[] = { 0, 1,
                           0, 2, 
                           0, 3,
                           1, 2,
                           1, 3, 
                           3, 1};
                           
        ///////////////////DYNAMIC RENDERER////////////
    glGenBuffers(1, &IBO_Lines);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO_Lines);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices), Indices, GL_DYNAMIC_DRAW);
    

    // Allocate space and upload the data from CPU to GPU
    glBindBuffer(GL_ARRAY_BUFFER, VBO_Lines);
    //glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_position), vertices_position, GL_DYNAMIC_DRAW);	
    glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(Vector3f), NULL, GL_DYNAMIC_DRAW);   

    //glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);


    // shaderProgram = create_program("shaders/vert.shader", "shaders/frag.shader");
    // GLint position_attribute = glGetAttribLocation(shaderProgram, "position");	// Get the location of the attributes that enters in the vertex shader
    // glVertexAttribPointer(position_attribute, 3, GL_FLOAT, GL_FALSE, 0, 0);	// Specify how the data for position can be accessed

    shaderProgram = create_program("shaders/shader.vs", "shaders/shader.fs"); //FROM FOR EXAMPLE OGLDEV TUT10
    

    // Enable the attribute
    //glEnableVertexAttribArray(position_attribute);

    // Enable point size
    glEnable(GL_PROGRAM_POINT_SIZE);
    
    //TODO, ASOCIATE WITH PARAMS 
    WINDOW_WIDTH = 800;
    WINDOW_HEIGHT = 600; 
   
    m_persProjInfo.FOV    = 60.0f;
    m_persProjInfo.Height = WINDOW_HEIGHT;
    m_persProjInfo.Width  = WINDOW_WIDTH;
    m_persProjInfo.zNear  = 0.5f;
    m_persProjInfo.zFar   = 100.0f;  
    
    //////////////////////// DYNAMIC RENDERER
    
    /// IF NOT DYNAMIC
  }   
  
}


//OLD (STATIC) ONE. FROM EXAMPLE ON OPENGL101
void Renderer::drawLines(){

      /////////////////// ORIGINAL SOLANA TUTORIAL WITH JUST DRAWARRAYS WITHOUT INDICES
  glDisable(GL_DEPTH_TEST); //LUCIANO; TO DRAW LINES; DISABLE THIS
	//glClear(GL_COLOR_BUFFER_BIT);

	glBindVertexArray(VAO_Lines);
	glDrawArrays(GL_POINTS, 0, 12);
  glDrawArrays(GL_LINES, 0, 12);
  
  glEnable(GL_DEPTH_TEST);
  
  //////////////////////// SEE TUTORIAL 10 OGLDEV /////////////////////////////////////

}


void Renderer::drawLines(/*Vector3f *Vertices, */
                          const float* Vertices, const float color[4], 
                                        int numPoints,   
                                        const unsigned int* indices, int numIndices, 
                                        float lineWidthIn, Matrix4f *WVP){
                                         

  glDisable(GL_DEPTH_TEST);

	float lineWidth = lineWidthIn;
	//b3Clamp(lineWidth, (float)lineWidthRange[0], (float)lineWidthRange[1]);
	glLineWidth(lineWidth);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); //VAO
  glBindBuffer(GL_ARRAY_BUFFER, 0);


	glUseProgram(shaderProgram); //IF DOES NOT THIS ->position_attribute = glGetAttribLocatio AND  glEnableVertexAttribArray(position_attribute);
  //COORDINATES ARE OF SCREEN

  // Pipeline p;

  // p.SetCamera(m_Camera->GetPos(), m_Camera->GetTarget(), m_Camera->GetUp());
  // //cout << "Camera pos "<<m_Camera->GetPos().x<<endl;
  // p.SetPerspectiveProj(m_persProjInfo);
  // //cout <<"Matrix"<<endl;

  //glUniformMatrix4fv(gWVPLocation, 1, GL_TRUE, (const GLfloat*)p.GetWVPTrans());
  gWVPLocation = glGetUniformLocation(shaderProgram, "gWVP"); //THIS CAN BE DONE ONCE 
  glUniformMatrix4fv(gWVPLocation, 1, GL_TRUE, (const GLfloat*)*WVP);
  colour = glGetUniformLocation(shaderProgram, "colour");
  glUniform4f(colour, color[0], color[1], color[2], color[3]);
  

  glBindVertexArray(VAO_Lines); //ORIGINALLY IT WAS LIKE

  //glEnableVertexAttribArray(0);
  

    
  GLint position_attribute = glGetAttribLocation(shaderProgram, "position");	// Get the location of the attributes that enters in the vertex shader
  // cout << "position attribute idx "<<position_attribute<<endl;
  	glEnableVertexAttribArray(position_attribute);
    glVertexAttribPointer(position_attribute, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0); //VAO POINTS

  // glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0); //VAO POINTS
  // glEnableVertexAttribArray(0);
 
  // IBO 
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO_Lines); //IBO
  //glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(indices), indices);  
  int indexBufferSizeInBytes = numIndices * sizeof(unsigned int);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufferSizeInBytes, indices, GL_DYNAMIC_DRAW);
  //
  //glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertices), Vertices);
 

   glBindBuffer(GL_ARRAY_BUFFER, VBO_Lines);
  //glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertices), Vertices);  
     glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_DYNAMIC_DRAW);  
     
  //glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, indices);
  glDrawElements(GL_LINES, numIndices, GL_UNSIGNED_INT, 0);

  glDisableVertexAttribArray(0);

	glBindVertexArray(0);	
	//glPointSize(1);
	glUseProgram(0);
  
  // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  // glBindBuffer(GL_ARRAY_BUFFER, 0);
  
  glEnable(GL_DEPTH_TEST);
  
  

}

  

// Read a shader source from a file
// store the shader source in a std::vector<char>
void read_shader_src(const char *fname, std::vector<char> &buffer) {
	std::ifstream in;
	in.open(fname, std::ios::binary);

	if(in.is_open()) {
		// Get the number of bytes stored in this file
		in.seekg(0, std::ios::end);
		size_t length = (size_t)in.tellg();

		// Go to start of the file
		in.seekg(0, std::ios::beg);

		// Read the content of the file in a buffer
		buffer.resize(length + 1);
		in.read(&buffer[0], length);
		in.close();
		// Add a valid C - string end
		buffer[length] = '\0';
	}
	else {
		std::cerr << "Unable to open " << fname << " I'm out!" << std::endl;
		//exit(-1);
	}
}

// Compile a shader
unsigned int load_and_compile_shader(const char *fname, GLenum shaderType) {
	// Load a shader from an external file
	std::vector<char> buffer;
	read_shader_src(fname, buffer);
	const char *src = &buffer[0];

	// Compile the shader
	GLuint shader = glCreateShader(shaderType);
	glShaderSource(shader, 1, &src, NULL);
	glCompileShader(shader);
	// Check the result of the compilation
	GLint test;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &test);
	if(!test) {
		std::cerr << "Shader compilation failed with this message:" << std::endl;
		std::vector<char> compilation_log(512);
		glGetShaderInfoLog(shader, compilation_log.size(), NULL, &compilation_log[0]);
		std::cerr << &compilation_log[0] << std::endl;
		//glfwTerminate();
		exit(-1);
	}
	return shader;
}

// Create a program from two shaders
unsigned int create_program(const char *path_vert_shader, const char *path_frag_shader) {
	// Load and compile the vertex and fragment shaders
	GLuint vertexShader = load_and_compile_shader(path_vert_shader, GL_VERTEX_SHADER);
	GLuint fragmentShader = load_and_compile_shader(path_frag_shader, GL_FRAGMENT_SHADER);

	// Attach the above shader to a program
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);

	// Flag the shaders for deletion
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// Link and use the program
	glLinkProgram(shaderProgram);
	glUseProgram(shaderProgram);

	return shaderProgram;
}



