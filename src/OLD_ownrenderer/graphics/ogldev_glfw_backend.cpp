/*
	Copyright 2014 Etay Meiri
*/

#ifdef WIN32
#include <Windows.h>
#endif
#include <stdio.h>
#include <glad/glad.h>
//#include <GL/glew.h>
//#define GLFW_DLL #IF LINKED STATICALLY THIS LINE SHOULD BE PRESENT

#include "ogldev_util.h"
#include "ogldev_glfw_backend.h"

#include <iostream>
using namespace std;

// Points to the object implementing the ICallbacks interface which was delivered to
// GLUTBackendRun(). All events are forwarded to this object.
static ICallbacks* s_pCallbacks = NULL;

static bool sWithDepth = false;
static bool sWithStencil = false;
//static GLFWwindow* s_pWindow = NULL;

GLFWwindow* s_pWindow = NULL;


static OGLDEV_KEY GLFWKeyToOGLDEVKey(uint Key)
{
    if (Key >= GLFW_KEY_SPACE && Key <= GLFW_KEY_RIGHT_BRACKET) {
        return (OGLDEV_KEY)Key;
    }
    
    switch (Key) {
        case GLFW_KEY_ESCAPE:            
            return OGLDEV_KEY_ESCAPE;
        case GLFW_KEY_ENTER:         
            return OGLDEV_KEY_ENTER;
        case GLFW_KEY_TAB:          
            return OGLDEV_KEY_TAB;
        case GLFW_KEY_BACKSPACE:  
            return OGLDEV_KEY_BACKSPACE;
        case GLFW_KEY_INSERT:         
            return OGLDEV_KEY_INSERT;
        case GLFW_KEY_DELETE:        
            return OGLDEV_KEY_DELETE;
        case GLFW_KEY_RIGHT:         
            return OGLDEV_KEY_RIGHT;
        case GLFW_KEY_LEFT:         
            return OGLDEV_KEY_LEFT;
        case GLFW_KEY_DOWN:        
            return OGLDEV_KEY_DOWN;            
        case GLFW_KEY_UP:         
            return OGLDEV_KEY_UP;
        case GLFW_KEY_PAGE_UP:   
            return OGLDEV_KEY_PAGE_UP;
        case GLFW_KEY_PAGE_DOWN:      
            return OGLDEV_KEY_PAGE_DOWN;
        case GLFW_KEY_HOME:    
            return OGLDEV_KEY_HOME;
        case GLFW_KEY_END:     
            return OGLDEV_KEY_END;
        case GLFW_KEY_F1:        
            return OGLDEV_KEY_F1;
        case GLFW_KEY_F2:        
            return OGLDEV_KEY_F2;
        case GLFW_KEY_F3:       
            return OGLDEV_KEY_F3;
        case GLFW_KEY_F4:   
            return OGLDEV_KEY_F4;
        case GLFW_KEY_F5:      
            return OGLDEV_KEY_F5;
        case GLFW_KEY_F6:     
            return OGLDEV_KEY_F6;
        case GLFW_KEY_F7:     
            return OGLDEV_KEY_F7;
        case GLFW_KEY_F8:     
            return OGLDEV_KEY_F8;
        case GLFW_KEY_F9:     
            return OGLDEV_KEY_F9;
        case GLFW_KEY_F10:    
            return OGLDEV_KEY_F10;
        case GLFW_KEY_F11:   
            return OGLDEV_KEY_F11;
        case GLFW_KEY_F12:    
            return OGLDEV_KEY_F12;
        // default:
            // OGLDEV_ERROR("Unimplemented OGLDEV key");
    }
    
    return OGLDEV_KEY_UNDEFINED;
}

static OGLDEV_MOUSE GLFWMouseToOGLDEVMouse(uint Button)
{
	switch (Button) {
	case GLFW_MOUSE_BUTTON_LEFT:
		return OGLDEV_MOUSE_BUTTON_LEFT;
	case GLFW_MOUSE_BUTTON_RIGHT:
		return OGLDEV_MOUSE_BUTTON_RIGHT;
	case GLFW_MOUSE_BUTTON_MIDDLE:
		return OGLDEV_MOUSE_BUTTON_MIDDLE;
	// default:
		// OGLDEV_ERROR("Unimplemented OGLDEV mouse button");
	}

	return OGLDEV_MOUSE_UNDEFINED;
}

static void KeyCallback(GLFWwindow* pWindow, int key, int scancode, int action, int mods)
{   
    OGLDEV_KEY OgldevKey = GLFWKeyToOGLDEVKey(key);
    
    s_pCallbacks->KeyboardCB(OgldevKey);
}


static void CursorPosCallback(GLFWwindow* pWindow, double x, double y)
{
    s_pCallbacks->PassiveMouseCB((int)x, (int)y);
}


static void MouseCallback(GLFWwindow* pWindow, int Button, int Action, int Mode)
{
    OGLDEV_MOUSE OgldevMouse = GLFWMouseToOGLDEVMouse(Button);

    OGLDEV_KEY_STATE State = (Action == GLFW_PRESS) ? OGLDEV_KEY_STATE_PRESS : OGLDEV_KEY_STATE_RELEASE;

    double x, y;

    glfwGetCursorPos(pWindow, &x, &y);

    s_pCallbacks->MouseCB(OgldevMouse, State, (int)x, (int)y);
}

static void InitCallbacks()
{
    glfwSetKeyCallback(s_pWindow, KeyCallback);
    glfwSetCursorPosCallback(s_pWindow, CursorPosCallback);
    glfwSetMouseButtonCallback(s_pWindow, MouseCallback);
}

void GLFWErrorCallback(int error, const char* description)
{
#ifdef WIN32
    char msg[1000];
    _snprintf_s(msg, sizeof(msg), "GLFW error %d - %s", error, description);
    MessageBoxA(NULL, msg, NULL, 0);
#else
    fprintf(stderr, "GLFW error %d - %s", error, description);
#endif    
    exit(0);
}

#include <iostream>
void GLFWBackendInit(int argc, char** argv, bool WithDepth, bool WithStencil)
{
    sWithDepth = WithDepth;
    sWithStencil = WithStencil;

    if (glfwInit() != 1) {
		std::cout << "Error initializing GLFW"<<std::endl;
        OGLDEV_ERROR("Error initializing GLFW");
        exit(1);
    }
    
    int Major, Minor, Rev;
    
    glfwGetVersion(&Major, &Minor, &Rev);
    
    printf("GLFW %d.%d.%d initialized\n", Major, Minor, Rev);
    
    glfwSetErrorCallback(GLFWErrorCallback);
}


void GLFWBackendTerminate()
{
    glfwDestroyWindow(s_pWindow);
    glfwTerminate();
}


bool GLFWBackendCreateWindow(uint Width, uint Height, bool isFullScreen, const char* pTitle)
{
    GLFWmonitor* pMonitor = isFullScreen ? glfwGetPrimaryMonitor() : NULL;

    s_pWindow = glfwCreateWindow(Width, Height, pTitle, pMonitor, NULL);

    if (!s_pWindow) {
        OGLDEV_ERROR("error creating window");
        exit(1);
    }
    
    glfwMakeContextCurrent(s_pWindow);
    
    // Must be done after glfw is initialized!
    // glewExperimental = GL_TRUE;
    // GLenum res = glewInit();
    // if (res != GLEW_OK) {
        // OGLDEV_ERROR((const char*)glewGetErrorString(res));
        // exit(1);
    // }    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }    
    return (s_pWindow != NULL);
}

void GLFWBackendRun(ICallbacks* pCallbacks)
{
    if (!pCallbacks) {
        OGLDEV_ERROR("callbacks not specified");
        exit(1);
    }
	
		//ORIGINAL
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glFrontFace(GL_CW);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
		//
		
		// //NEW FOR TEXT
    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		// ///
		
		////THIS WAS IN THE ORIGINAL
    if (sWithDepth) {
        glEnable(GL_DEPTH_TEST);
    }
    //glEnable(GL_MULTISAMPLE);

    s_pCallbacks = pCallbacks;
    InitCallbacks();

    while (!glfwWindowShouldClose(s_pWindow)) {
			//glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
      glClearColor(0.f, 0.f, 0.f, 1.0f);
        s_pCallbacks->RenderSceneCB();        
        glfwSwapBuffers(s_pWindow);
        glfwPollEvents();
		
		//GLFWBackendProcessInput();
		
		//if (glfwGetKey(s_pWindow, GLFW_KEY_W) == GLFW_RELEASE) 	cout << "W released! "<<endl;
    }
}

// void GLFWBackendProcessInput(){
	// if (glfwGetKey(s_pWindow, GLFW_KEY_W) == GLFW_PRESS) 	cout << "W pressed !"<<endl;
// }

void GLFWBackendSwapBuffers()
{
    // Nothing to do here
}


void GLFWBackendLeaveMainLoop()
{
    glfwSetWindowShouldClose(s_pWindow, 1);
}
