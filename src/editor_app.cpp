#include "editor_app.h"

bool EditorApp::Init(){
  Vector3f Pos(0.0f, 5.0f, -22.0f);
  Vector3f Target(0.0f, -0.2f, 1.0f);
  Vector3f Up(0.0, 1.0f, 0.0f);

  m_pGameCamera = new Camera(SCR_WIDTH, SCR_HEIGHT, Pos, Target, Up);


  ////////////////////////////////////// 
  
  
    // // build and compile our shader program
  // // ------------------------------------
  // // vertex shader
  // unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
  // glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
  // glCompileShader(vertexShader);
  // // check for shader compile errors
  // int success;
  // char infoLog[512];
  // glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
  // if (!success)
  // {
      // glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
      // std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
  // }
  // // fragment shader
  // unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  // glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
  // glCompileShader(fragmentShader);
  // // check for shader compile errors
  // glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
  // if (!success)
  // {
      // glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
      // std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
  // }
  // // link shaders
  // shaderProgram = glCreateProgram();
  // glAttachShader(shaderProgram, vertexShader);
  // glAttachShader(shaderProgram, fragmentShader);
  // glLinkProgram(shaderProgram);
  // // check for linking errors
  // glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  // if (!success) {
      // glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
      // std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
  // }
  // glDeleteShader(vertexShader);
  // glDeleteShader(fragmentShader);


  // Vector3f Pos(1.0f, 3.0f, -5.0f);
  // Vector3f Target(-0.5f, -0.5f, 1.0f);
  // Vector3f Up(0.0, 1.0f, 0.0f);

  // camera = new Camera(SCR_WIDTH, SCR_HEIGHT, Pos, Target, Up);

  // Pipeline p;
  // m_persProjInfo.FOV    = 60.0f;
  // m_persProjInfo.Height = SCR_HEIGHT;
  // m_persProjInfo.Width  = SCR_WIDTH;
  // m_persProjInfo.zNear  = 0.02f;
  // m_persProjInfo.zFar   = 100.0f;  
  
  
  // cout << "Loading ground"<<endl;
  // LoadGround(&ground_mesh);

  // /// NOW LIGHT TECHNIQUE
  // m_plightEffect = new BasicLightingTechnique();

  // if (!m_plightEffect->Init()) {
  // printf("Error initializing the lighting technique\n");
  // return false;
  // }

  // m_plightEffect->Enable();

  // DirectionalLight 					m_directionalLight;
  // m_directionalLight.Color = Vector3f(1.0f, 1.0f, 1.0f);
  // m_directionalLight.AmbientIntensity = 0.55f;
  // m_directionalLight.DiffuseIntensity = 0.9f;
  // m_directionalLight.Direction = Vector3f(1.0f, 0.0, 0.0);


  // m_plightEffect->SetColorTextureUnit(COLOR_TEXTURE_UNIT_INDEX);
  // m_plightEffect->SetDirectionalLight(m_directionalLight);
  // m_plightEffect->SetMatSpecularIntensity(0.0f);
  // m_plightEffect->SetMatSpecularPower(0);

  // cout << "Creating plane"<<endl;
  // //AT LAST, PHYSICS
  // //Point and normal
  // Vector3f point(1.,0.,1.);
  // Vector3f normal(0.,1.,0.);
  // m_plane = new Plane(point,normal);

  // Node* n0 = new Node(Vector3f(0.,2.,0.));  
  // Node* n1 = new Node(Vector3f(0.,5.,0.));
  // Node* n2 = new Node(Vector3f(1.,5.,1.));
  // Node* n3 = new Node(Vector3f(-1.,5.,1.));



  // ////// Setup Dear ImGui context

  // glfwSwapInterval(1); // Enable vsync
  // IMGUI_CHECKVERSION();
  // ImGui::CreateContext();
  // ImGuiIO& io = ImGui::GetIO(); (void)io;
  // ////io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
  // ////io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

  // // Setup Dear ImGui style
  // ImGui::StyleColorsDark();
  // //ImGui::StyleColorsClassic();

  // const char* glsl_version = "#version 130";
  // glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  // glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  // //// Setup Platform/Renderer backends
  // ImGui_ImplGlfw_InitForOpenGL(window, true);
  // ImGui_ImplOpenGL3_Init(glsl_version);
  
  // cout << "GUI done"<<endl;


  // m_Text = new TextRenderer(SCR_WIDTH,SCR_HEIGHT); //CRASH

  // // uncomment this call to draw in wireframe polygons.
  // //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

  // // render loop
  // // -----------

  // float c_dot;
  // Vector3f vold;

  // float runtime;
  // int framecount=0;
  // long long frame_time;

  // float vnew [4];
  // bool impact = false;

  // m_start_time = GetCurrentTimeMillis();
  // long long frame_count = 0;

  // //For text 
  // // glEnable(GL_BLEND);
  // // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);



  // float kin_energy;

  
  // mesh_loaded = false;

  // if (!m_pickingTexture.Init(SCR_WIDTH, SCR_HEIGHT)) {
      // return false;
  // }

  // if (!m_pickingEffect.Init()) {
      // return false;
  // }
  
  
  // st = new Structure();
  // //First create the structure, then the inside members
  // st->LoadFromFile("struct.str");
  
  // for (int i=0;i<st->GetNodeCount();i++){
    // st->GetNode(i)->m_mass = 1.0;
  // }
  // for (int t =0; t<st->GetTrussCount();t++){
    // st->GetTruss(t)->m_K = 1.e3;
    // st->GetTruss(t)->m_C = 1.e-1;
    // st->GetTruss(t)->m_max_negtress = 250.;
  // }

  
  // is_struct = true;
  // cout << "Nodes "<<st->GetNodeCount()<<endl;
  // for (int i=0;i<st->GetNodeCount();i++) {
    // myMesh *mesh = new myMesh();
      // if (!mesh->LoadMesh(
        // "Sphere.dae"
        // )) {
        // std::cout<<"Mesh load failed"<<endl;
        // printf("Mesh load failed\n");

      // }
      // else {
        // m_nodemesh.push_back(mesh);
        // mesh_loaded = true;
      // }
  // }
    // st->ApplyVel(Vector3f(0.,0.,0.));
    // st->ResetForces();  //Crucial
  
  
  
  // //FOR RENDER LINES
    // glGenVertexArrays(1, &VAO);
    // glGenBuffers(1, &VBO);
    // glGenBuffers(1, &EBO);
    // // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    // glBindVertexArray(VAO);

    // glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_DYNAMIC_DRAW);

    // glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    // glEnableVertexAttribArray(0);

    // // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    // glBindBuffer(GL_ARRAY_BUFFER, 0); 

    // // remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
    // //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    // glBindVertexArray(0);   

    // glFrontFace(GL_CW);
    // glCullFace(GL_BACK);
    // glEnable(GL_CULL_FACE);
    
    
  // ///////////////// TEXT ////
  // m_Text = new TextRenderer(SCR_WIDTH,SCR_HEIGHT); //CRASH
  
  // //Like OgldevApp
  // m_frameCount = 0;
  // m_frameTime = 0;
  // m_fps = 0;
  
  // m_is_node_sel = false;

  // m_frameTime = m_startTime = GetCurrentTimeMillis();
  
  // m_impact_force = 0.;
            
  
  
  return true;
}

     void EditorApp::RenderSceneCB(){}

	 void EditorApp::KeyboardCB(int OgldevKey){}
	
	 void EditorApp::ShadowMapPass(){}
	 void EditorApp::RenderPass(){}