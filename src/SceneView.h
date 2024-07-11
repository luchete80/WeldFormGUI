#include "FrameBuffer.h"

#include <iostream>
using namespace std;
// Like in Lumix
class SceneView {
public:
  SceneView(){
    m_framebuffer = NULL;
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