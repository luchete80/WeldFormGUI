#include "ViewportWindow.h"

#include "editor.h"

ViewportWindow::ViewportWindow(Editor *ed){
  m_editor = ed;
  
}



void ViewportWindow::Draw()
{
	// ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0.0f, 0.0f ) );
	// ImGui::Begin( "Viewport" );

	if ( HandleWindowResize() == false )
	{
		ImGui::End();
		ImGui::PopStyleVar();
		return;
	}

 
  //ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_NoScrollWithMouse);

  //if (ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_NoScrollWithMouse)) 
 /* 
  {
    m_sceneview->getFrameBuffer()->Bind();
    // RENDER SCENE

    
    
    scr_width = ImGui::GetContentRegionAvail().x;
    scr_height = ImGui::GetContentRegionAvail().y;
    m_editor->getSceneView()->getFrameBuffer()->RescaleFrameBuffer(scr_width, scr_height);
    ImVec2 vmin = ImGui::GetWindowContentRegionMin();
    cout << "win pos min "<< vmin.x<< ", "<<vmin.y <<endl;

    ImVec2 vmax = ImGui::GetWindowContentRegionMax();
    cout << "win pos max "<< vmax.x<< ", "<<vmax.y <<endl;
    
    //cout << "SCR WIDTH: "<<scr_width<< ", HEIGHT: "<<scr_height<<endl;
    // // *m_width = width;
    // // *m_height = height;
    ImGui::Image(
      (ImTextureID)m_sceneview->getFrameBuffer()->getFrameTexture(), 
      ImGui::GetContentRegionAvail(), 
      ImVec2(0, 1), 
      ImVec2(1, 0)
    );  

          // // // and here we can add our created texture as image to ImGui
          // // // unfortunately we need to use the cast to void* or I didn't find another way tbh
          // // ImGui::GetWindowDrawList()->AddImage(
              // // (void *)texture_id, 
              // // ImVec2(pos.x, pos.y), 
              // // ImVec2(pos.x + window_width, pos.y + window_height), 
              // // ImVec2(0, 1), 
              // // ImVec2(1, 0)
          // // );
    

    /////////////////////////////////////// CAMERA THINGS
    Pipeline pip;

    Pipeline pn;
    // pn.SetCamera(camera->GetPos(), camera->GetTarget(), camera->GetUp());
    // pn.SetPerspectiveProj(m_persProjInfo);
    // Matrix4f proj = pn.GetProjTrans();
    // glm::mat4 ptest = Matrix4fToGLM(proj);
    // render
    // ------
    glClearColor(0.25f, 0.25f, 0.25f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // draw our first triangle
    glUseProgram(m_editor->shaderProgram);
    gWVPLocation = glGetUniformLocation(m_editor->shaderProgram, "gWVP");
    
    
    //RENDERING FIRST IT WORKS; AFTER DOESn'T

      // m_plightEffect->Enable();
      // m_plightEffect->SetEyeWorldPos(camera->GetPos());
      // m_plightEffect->SetWVP(p.GetWVPTrans());
    
    Vector3f lightPos(1.2f, 1.0f, 2.0f);
    Vector3f lightColor(1.0f, 1.0f, 1.0f);
    Vector3f objectColor(1.0f, 0.5f, 0.31f);
      
      

      
     glUniform3fv(glGetUniformLocation(m_editor->shaderProgram, "lightPos"), 1, &lightPos[0]); 
     glUniform3fv(glGetUniformLocation(m_editor->shaderProgram, "lightColor"), 1, &lightColor[0]); 
     
           // lightingShader.setVec3("objectColor", 1.0f, 0.5f, 0.31f);
          // lightingShader.setVec3("lightColor", 1.0f, 1.0f, 1.0f);
          // lightingShader.setVec3("lightPos", lightPos);
          
      
      if (m_editor->is_sph_mesh){
        Matrix4f *mi = new Matrix4f[m_editor->m_domain.Particles.size()];
        for (int p=0;p<m_editor->m_domain.Particles.size();p++){    
        float h = m_editor->m_domain.Particles[0]->h/2.;

          Vec3_t v = m_domain.Particles[p]->x ;
          //Vector3f pos(v(0)*10.0,v(1)*10.0,v(2)*10.0); //ORTHO
          Vector3f pos(v(0),v(1),v(2)); 
          glm::mat4 model = glm::mat4(1.0f);
         // model[0][0]=model[1][1]=model[2][2]=h;
          //model[0][3] = -m_domain_center.x; model[1][3] = -m_domain_center.y; model[2][3] = -m_domain_center.z;
          //model[0][3] = -pos.x; model[1][3] = -pos.y; model[2][3] = -pos.z;   
          ////FIRST TRANSLATE AND THEN SCALE!!!!!
          model = glm::translate(model, glm::vec3(-m_editor->m_domain_center.x+pos.x,-m_editor->m_domain_center.y+pos.y,-m_editor->m_domain_center.z+pos.z));

          
          glm::mat4 projection(1.0);
          //projection = glm::perspective(glm::radians(60.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
          projection = glm::perspective(glm::radians(60.0f), (float)scr_width / (float)scr_height, 0.1f, 100.0f);
          // projection = glm::ortho(-0.0f , 800.0f / 2.0f, 
            // 600.0f / 2.0f, 0.0f, 
          // -1000.0f, 1000.0f);/////glm::ortho(xmin, xmax, ymin, ymax)
          //projection[0][0] = (float)SCR_HEIGHT/SCR_WIDTH;
          glm::mat4 view = glm::mat4(1.0f);// this command must be in the loop. Otherwise, the object moves if there is a glm::rotate func in the lop.    
          view = glm::translate(view, m_editor->arcCamera->position);// this, too.  
          view = glm::rotate(view, glm::radians(m_editor->arcCamera->angle), m_editor->arcCamera->rotationalAxis);
          
          glm::mat4 transback = glm::mat4(1.0f);
          transback = glm::translate(transback, glm::vec3(0.0,0.0,zcam));

          glm::mat4 mat = projection * transback * view * model;
          trans_mat [p]= mat;
  
          //m_plightEffect->SetWVP(pn.GetWVPTrans()); If wanted to rotate spheres
          //If personalized shader
          objectColor = Vector3f(0.0f, 0.5f, 1.0f);
          for (int s=0;s<m_editor->m_sel_count;s++){
            //cout << "sel_count"<<m_sel_count<<endl;
            if (p==m_sel_particles[s]) {objectColor = Vector3f(1.0f, 0.0f, 0.031f);

            }
            //else                    objectColor = Vector3f(0.0f, 0.5f, 1.0f);

          }
          glUniform3fv(glGetUniformLocation(m_editor->shaderProgram, "objectColor"), 1, &objectColor[0]); 

          glUniformMatrix4fv(gWVPLocation, 1, GL_FALSE, &mat[0][0]); ///// WITH GLM IS FALSE!!!!!!! (NOT TRANSPOSE)
          m_renderer.Render();
        }//loop particles

      
      } else if (is_fem_mesh) {
        
        ////// MESH RENDER 
        Vector3f pos(0.0,0.0,0.0); 
        glm::mat4 model = glm::mat4(1.0f);

        //model = glm::translate(model, glm::vec3(-m_domain_center.x+pos.x,-m_domain_center.y+pos.y,-m_domain_center.z+pos.z));
        
        model = glm::translate(model, glm::vec3(-m_editor->m_femsh_center.x+pos.x,-m_editor->m_femsh_center.y+pos.y,-m_editor->m_femsh_center.z+pos.z));
                
        glm::mat4 projection(1.0);
        //projection = glm::perspective(glm::radians(60.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        projection = glm::perspective(glm::radians(60.0f), (float)scr_width / (float)scr_height, 0.1f, 100.0f);
        
        glm::mat4 view = glm::mat4(1.0f);// this command must be in the loop. Otherwise, the object moves if there is a glm::rotate func in the lop.    
        view = glm::translate(view, m_editor->arcCamera->position);// this, too.  
        view = glm::rotate(view, glm::radians(m_editor->arcCamera->angle), arcCamera->rotationalAxis);
        
        glm::mat4 transback = glm::mat4(1.0f);
        transback = glm::translate(transback, glm::vec3(0.0,0.0,zcam));

        glm::mat4 mat = projection * transback * view * model;


        pn.WorldPos(pos);   
        //Matrix4f m = pn.GetWVPTrans(); //NOT USED!


        objectColor = Vector3f(0.0f, 0.5f, 1.0f);
        // for (int s=0;s<m_sel_count;s++){

        glUniform3fv(glGetUniformLocation(shaderProgram, "objectColor"), 1, &objectColor[0]); 

        //trans_mat [p]= mat;
     
        glUniformMatrix4fv(gWVPLocation, 1, GL_FALSE, &mat[0][0]); ///// WITH GLM IS FALSE!!!!!!! (NOT TRANSPOSE)
        m_editor->m_renderer.Render();

        //TODO: CHANGE TO INSTANCING
        ////RENDER every NODES 
        /////// TODO: PASS THIS TO RENDERER
        for (int p=0;p<m_editor->m_fem_msh->getNodeCount();p++){ 


        }
        m_editor->m_renderer.Render(); //THIS IS DONE ONCE      
        } //IF IS FEM

                      // top left
            // bottom left
            // top right
            // bottom right
              glBegin (GL_QUADS);

                glVertex2f (-0.1,0.1);
                glVertex2f (-0.1,-0.1);
                glVertex2f (0.1,-0.1);
                glVertex2f (0.1,0.1);
              

              glEnd ();

    glUseProgram(0);

    m_sceneview->getFrameBuffer()->Unbind();
    
  }
  
  

  ImGui::End();  
  */
  
  
}

bool ViewportWindow::HandleWindowResize()
{
	// ImVec2 view = ImGui::GetContentRegionAvail();

	// if ( view.x != m_Window.width || view.y != m_Window.height )
	// {
		// if ( view.x == 0 || view.y == 0 )
		// {
			// // The window is too small or collapsed.
			// return false;
		// }

		// m_Window.width = view.x;
		// m_Window.height = view.y;

		// //RecreateFramebuffer();

		// // The window state has been successfully changed.
		// return true;
	// }

	// The window state has not changed.
	return false;
}
