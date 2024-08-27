
#include <iostream>
#include <map>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "filesystem.h"

#include <ft2build.h>
#include FT_FREETYPE_H
  
#ifndef WIN32
#include <sys/time.h>
#include <unistd.h>
#endif
#include <sys/types.h>

#include "editor_app.h"

#include <iostream>
using namespace std;

  
#include "text_renderer.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

int main(int argc, char** argv) {

	std::cout << "Hi "<<endl;
	
	GLFWBackendInit(argc, argv, true, false);
	if (!GLFWBackendCreateWindow(800, 600, false, "Protoball")){
        std::cout << "cannot create"<<endl;
		return 1;		
	}
 
  std::cout << "context created"<<endl;
  //SRANDOM;
  
  EditorApp* pApp;
	cout << "creating app app"<<endl;
	pApp= new EditorApp();
	cout << "created app"<<endl;
    if (!pApp->Init()) {
		cout << "failed"<<endl;
		GLFWBackendTerminate();
         return 1;
    }
    cout << "running"<<endl;
    pApp->Run();

    delete pApp;
	GLFWBackendTerminate();
 
    return 0;
}
