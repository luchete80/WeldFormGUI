//https://uysalaltas.github.io/2022/01/09/OpenGL_Imgui.html
//https://www.codingwiththomas.com/blog/rendering-an-opengl-framebuffer-into-a-dear-imgui-window
#ifndef _SCENEVIEW_H_
#define _SCENEVIEW_H_

#include "FrameBuffer.h"


#include <iostream>
using namespace std;
// Like in Lumix
class SceneView {
public:
  SceneView(){
    m_framebuffer = NULL;
    float m_pos_x,m_pos_y;
  }
  SceneView(const unsigned int &width, const unsigned int &height){
    
    cout << "Creating framebuffer"<<endl;
    if (m_framebuffer!=NULL)
      m_framebuffer = new FrameBuffer(width, height);
    else
      cout << "Null Framebuffer"<<endl;
  }

  FrameBuffer* getFrameBuffer(){return m_framebuffer;};
protected:  
  FrameBuffer *m_framebuffer; 

};    
#endif