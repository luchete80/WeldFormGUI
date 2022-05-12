#ifndef _EDITOR_APP_H
#define _EDITOR_APP_H


#include "ogldev_engine_common.h"
#include "ogldev_app.h"
#include "ogldev_camera.h"
#include "ogldev_util.h"
#include "ogldev_pipeline.h"
#include "ogldev_camera.h"
#include "assimp/texture.h"
#include "skinning_technique.h"
#include "ogldev_glfw_backend.h"
#include "ogldev_skinned_mesh.h"

#include "ogldev_basic_lighting.h"

#include "ogldev_shadow_map_fbo.h"
#include "shadow_map_technique.h"
#include "ogldev_util.h"

#include "text_renderer.h"

//#include "ogldev_basic_mesh.h"
#include "myMesh.h"

class EditorApp : 
public ICallbacks, 
public OgldevApp
{
public:

    EditorApp(){};

    virtual ~EditorApp()
    {
        // SAFE_DELETE(m_pEffect);
        // SAFE_DELETE(m_pGameCamera);
    }    

    bool Init();

    void Run() {
      GLFWBackendRun(this);
    }
	
	
    

    virtual void RenderSceneCB();

	virtual void KeyboardCB(int OgldevKey);
	
	virtual void ShadowMapPass();
	virtual void RenderPass();

	virtual void PassiveMouseCB(int x, int y)
	{
		m_pGameCamera->OnMouse(x, y);
	}
    
 
private:      
 
  Camera* 									m_pGameCamera;
  DirectionalLight 					m_directionalLight;

  Vector3f 										m_position;            
  PersProjInfo 								m_persProjInfo;
	myMesh 											m_fieldmesh;
  BasicLightingTechnique* 		m_plightEffect;	
	std::vector<SkinnedMesh*> 	m_playermesh;
	myMesh*											m_ballmesh;
	
	myMesh*											m_goalmesh[2];
	
	bool WASD_down[4];
	TextRenderer*							m_Text;
	//PrecisionTimer timer(Prm.FrameRate);
	// PrecisionTimer 						timer;
	
	// //std::chrono::duration<double> last_update_time,current_update_time;
	// std::chrono::steady_clock::time_point last_update_time,current_update_time;
	
	
	//SHADOW
	ShadowMapFBO 							m_shadowMapFBO;
	ShadowMapTechnique* 			m_pShadowMapEffect;
	
	//clock_t last_elapsed_clock;
  int SCR_WIDTH ;
  int SCR_HEIGHT ;
	

};

#endif