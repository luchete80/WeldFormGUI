//EXAMPLE FROM https://github.com/ocornut/imgui/issues/1287
#ifndef _VIEWPORT_H_
#define _VIEWPORT_H_

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "SceneView.h"

class Editor;

class ViewportWindow {
public :
  ViewportWindow(){ m_sceneview = NULL;}
  ViewportWindow(Editor *ed);
  void setScene(SceneView* scv){m_sceneview = scv;}
  void Draw();
  bool HandleWindowResize();
  const ImVec2 & getContentRegionMin()const {return vmin;}

protected:
  ImVec2 vmin, vmax;
  SceneView* m_sceneview;
  Editor *m_editor;
  
  float scr_width,scr_height;
  
};



#endif