#ifndef _SCENE_H_
#define _SCENE_H_

class Renderer;

class SceneNode {
public:
  SceneNode(){}
  
private:
  friend class Renderer;
  
};

#endif