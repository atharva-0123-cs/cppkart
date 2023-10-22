#include "XJZoomEngine.h"

// Windowing consts
#define WinWidth 1800
#define WinHeight 1000

std::vector<GLfloat> vertices = {};
std::vector<GLuint> indices = {};

ObjModel firstCarModel = ObjModel("../src/ressources/first_car.obj");
ObjModel firstCarWheelModel = ObjModel("../src/ressources/first_car_wheel.obj");

std::vector<GLfloat> playerVehicle_verts = firstCarModel.GetVertices();
std::vector<GLuint> playerVehicle_indices = firstCarModel.GetIndices();

std::vector<GLfloat> VW_vertices = firstCarWheelModel.GetVertices();
std::vector<GLuint> VW_indices = firstCarWheelModel.GetIndices();

// Terrain Scale matrix


glm::mat4 WheelMatrix = glm::scale(glm::vec3(0.25f, 0.25f, 0.25f));

glm::mat4 terrainModelMatrix = glm::scale(glm::vec3(10.0f, 1.0f, 10.0f));
glm::mat4 vehicleModelMatrix = glm::scale(glm::vec3(0.25f));

void XJZoomEngine::Run()
{
  int carTexWidth, carTexHeight, carTexChannels;
  auto carTextureData = stbi_load("../src/ressources/first_car.png", &carTexWidth, &carTexHeight, &carTexChannels, STBI_rgb_alpha);
  assert(carTextureData);

  //* ### Bullet Physics World Singleton Instanciation ###

  PhysicsWorldSingleton *physicsWorld = PhysicsWorldSingleton::getInstance();

  //* ############ PROTOTYPE Collision Plane ############

  btTransform protoPlaneTransform;
  protoPlaneTransform.setIdentity();
  protoPlaneTransform.setOrigin(btVector3(0, 0, 0));
  btStaticPlaneShape *plane = new btStaticPlaneShape(btVector3(0, 1, 0), btScalar(0));

  // Create Motion shape:
  btMotionState *motion = new btDefaultMotionState(protoPlaneTransform); //! He put btDefaultMotionShape

  btRigidBody::btRigidBodyConstructionInfo info(0.0, motion, plane);
  info.m_friction = 0.6f;
  btRigidBody *planeBody = new btRigidBody(info);
  physicsWorld->dynamicsWorld->addRigidBody(planeBody);

  //* ############ PROTOTYPE Collision Plane ^^^ ############

  VehicleEntity vehicle;

  // Terrain Physics
  int width;
  int length;
  std::vector<unsigned short> heightDataVec; // Provide your actual height data here
  btScalar minHeight;                        // Minimum height in your dataset
  btScalar maxHeight;                        // Maximum height in your dataset

  bool loadTerrainFromIMG = loadHeightfieldData("../src/ressources/track1.png", heightDataVec, width, length, minHeight, maxHeight);

  unsigned short *heightData = new unsigned short[heightDataVec.size()];
  std::copy(heightDataVec.begin(), heightDataVec.end(), heightData);

  TerrainPhysics terrain(width, length, heightData, minHeight, maxHeight);

  //* Add terrain to physics world
  // physicsWorld->dynamicsWorld->addRigidBody(terrain.GetRigidBody());

  //* ########## WINDOWING STUFF ############
  uint32_t WindowFlags = SDL_WINDOW_OPENGL;
  SDL_Window *Window = SDL_CreateWindow("XJZoom Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WinWidth, WinHeight, WindowFlags);
  assert(Window);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GLContext Context = SDL_GL_CreateContext(Window);
  double prevTime = SDL_GetTicks(); //Window Tick Rate (SDL thing)

  gladLoadGLLoader(SDL_GL_GetProcAddress);

  GLuint carTexID;
  glGenTextures(1, &carTexID);
  glBindTexture(GL_TEXTURE_2D, carTexID);

  // Depending on the number of channels, pick the appropriate format
  GLenum carTexFormat;
  if (carTexChannels == 1) carTexFormat = GL_RED;
  else if (carTexChannels == 3) carTexFormat = GL_RGB;
  else if (carTexChannels == 4) carTexFormat = GL_RGBA;

  //* Some GL config..

  glTexImage2D(GL_TEXTURE_2D, 0, carTexFormat, carTexWidth, carTexHeight, 0, carTexFormat, GL_UNSIGNED_BYTE, carTextureData);
  glGenerateMipmap(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  int32_t Running = 1;
  int32_t FullScreen = 0;

  // Specify the viewport of OpenGL in the Window
  // In this case the viewport goes from x = 0, y = 0, to x = 800, y = 800
  glViewport(0, 0, WinWidth, WinHeight);
  glEnable(GL_DEPTH_TEST);

  // Load Managers:
  SceneManager sceneManager;

  terrainMapLoader(indices, vertices);

  // Generates Shader object using shaders default.vert and default.frag
  Shader shaderProgram("../src/rendering/shader/default.vert", "../src/rendering/shader/default.frag");

  //*ModelMatrix for GLSL Shader
  auto modelMatrixLocation = glGetUniformLocation(shaderProgram.ID, "modelMatrix");


  //* ####### Terrain Geometry Instance
  VAO VAO1; VAO1.Bind();
  VBO VBO1(vertices); //Vertex Buffer Object ; links it to vertices
  EBO EBO1(indices); //Element Buffer Object ; links it to indices
  RenderableGeometry terrainGeom(&VAO1, &VBO1, &EBO1, vertices, indices);


  //* ####### Player Vehicle Geometry Instance
  VAO VAO2; VAO2.Bind();
  VBO VBO2(playerVehicle_verts);
  EBO EBO2(playerVehicle_indices);
  RenderableGeometry vehicleBodyGeom(&VAO2, &VBO2, &EBO2, playerVehicle_verts, playerVehicle_indices);

  //* ####### Wheel Geom Instance
  VAO VAO3; VAO3.Bind();
  VBO VBO3(VW_vertices);
  EBO EBO3(VW_indices);
  RenderableGeometry vehicleWheelGeom(&VAO3, &VBO3, &EBO3, VW_vertices, VW_indices);


  GLuint uniID = glGetUniformLocation(shaderProgram.ID, "scale");

  std::string texPath = "../src/ressources/";

  // !Texture, TODO ground texture, for terrain
  // Texture brickTex((texPath + "brick.png").c_str(), GL_TEXTURE_2D, GL_TEXTURE0, GL_RGBA, GL_UNSIGNED_BYTE);
  // brickTex.texUnit(shaderProgram, "tex0", 0);
  // Enables the Depth Buffer


  // Creates camera object
  Camera camera(WinWidth, WinHeight, glm::vec3(0.0f, 0.0f, 2.0f));

  // #### MAIN GAME LOOP THAT ENGINE IS RUNNING:
  while (Running)
  {

    //*Idle Forces when no keys are applied:
    vehicle.GetPhysics().ApplyEngineForce(0);
    vehicle.GetPhysics().ApplySteer(0);

    SDL_Event Event;
    while (SDL_PollEvent(&Event))
    {
      if (Event.type == SDL_KEYDOWN)
      {
        switch (Event.key.keysym.sym)
        {
        case SDLK_ESCAPE:
          Running = 0;
          break;
        case 'f':
          FullScreen = !FullScreen;
          if (FullScreen)
          {
            SDL_SetWindowFullscreen(Window, WindowFlags | SDL_WINDOW_FULLSCREEN_DESKTOP);
          }
          else
          {
            SDL_SetWindowFullscreen(Window, WindowFlags);
          }
          break;
        case SDLK_UP:
          vehicle.GetPhysics().ApplyEngineForce(7000);
          break;
        case SDLK_DOWN:
          vehicle.GetPhysics().ApplyEngineForce(-2500);
          break;
        case SDLK_RIGHT:
          vehicle.GetPhysics().ApplySteer(-0.3);
          break;
        case SDLK_LEFT:
          vehicle.GetPhysics().ApplySteer(0.3);
          break;
        default:
          break;
        }
      }
      else if (Event.type == SDL_QUIT)
      {
        Running = 0;
      }
    }

    //! ### IMPORTANT, Allow the Physics Simulation to tick ###
    physicsWorld->dynamicsWorld->stepSimulation(1.0f / 60.0f);

    //! PROTOTYPING: VEHICLE RENDERING CODE
    // ### Update the box's model matrix to match the vehicle's transform ###
    btTransform vehicleTransform = vehicle.GetPhysics().GetTransform();

    btVector3 vehiclePosition = vehicleTransform.getOrigin();
    btQuaternion vehicleRotation = vehicleTransform.getRotation();

    glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(vehiclePosition.x(), vehiclePosition.y(), vehiclePosition.z()));
    glm::mat4 rotation = glm::mat4_cast(glm::quat(vehicleRotation.w(), vehicleRotation.x(), vehicleRotation.y(), vehicleRotation.z()));
    glm::mat4 rotate90DEG_Adjustment = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    //* Wheel Model Matrices
    std::vector<glm::mat4> wheelMatrices(4);

    //for (int i = 0; i < 1; i++)
     for (int i = 0; i < vehicle.GetPhysics().vehicle->getNumWheels(); i++)
    {
        btWheelInfo wheelinfo = vehicle.GetPhysics().vehicle->getWheelInfo(i);
      
        //* Get Translation (Positioning)
        float wX = wheelinfo.m_worldTransform.getOrigin().getX();
        float wY = wheelinfo.m_worldTransform.getOrigin().getY();
        float wZ = wheelinfo.m_worldTransform.getOrigin().getZ();

        glm::mat4 wheelTranslation = glm::translate(glm::mat4(1.0f), glm::vec3(wX,wY,wZ));

        //* Extract only the rotation around the axle (e.g., Y-axis)
        btTransform wheelTransform = wheelinfo.m_worldTransform;
        btScalar yaw, pitch, roll;
        wheelTransform.getBasis().getEulerZYX(yaw, pitch, roll);

    // Convert yaw to a quaternion (assuming yaw is around Y-axis)
    glm::quat glmYawQuat = glm::angleAxis(glm::degrees(yaw), glm::vec3(0, -1, 0)); // Convert yaw from radians to degrees

    glm::mat4 wheelRotation = glm::toMat4(glmYawQuat);

    // Full transformation matrix for the wheel
      wheelMatrices.push_back(
        wheelTranslation * wheelRotation * rotate90DEG_Adjustment * glm::scale(glm::vec3(0.25f))
        //wheelTranslation * wheelRotation  * glm::scale(glm::vec3(0.25f))
      );
    }


  //
    vehicleModelMatrix = translation * rotation * rotate90DEG_Adjustment * glm::scale(glm::vec3(0.4f));
    //WheelMatrix = translation * glm::scale(glm::vec3(0.25f));

    //! PROTOTYPING: VEHICLE RENDERING CODE

    // Specify the color of the background
    glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
    // Clean the back buffer and depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // Tell OpenGL which Shader Program we want to use
    shaderProgram.Activate();

    // Assigns a value to the uniform; NOTE: Must always be done after activating the Shader Program
    // brickTex.Bind();

    //* ###### CAMERA #######

    //* naive approach (hard offsets camera, bad for steering)
    //  camera.Position.x = vehiclePosition.x() + 0.5f;
    //  camera.Position.y = vehiclePosition.y() + 2.0f;
    //  camera.Position.z = vehiclePosition.z() - 3.0f;

    //* #### Smooth Camera (For Driving)
    auto targetVec = glm::vec3(vehiclePosition.x() + 0.4f, vehiclePosition.y() + 1.3f, vehiclePosition.z() - 2.4f);
    auto dirVec = targetVec - camera.Position;
    if(glm::distance2(targetVec, camera.Position) > 0.02f)
    camera.Position += dirVec * 0.03f;
    camera.LookAt.x = vehiclePosition.x();
    camera.LookAt.y = vehiclePosition.y();
    camera.LookAt.z = vehiclePosition.z();

    //camera.Inputs(Window);

    //  Updates and exports the camera matrix to the Vertex Shader
    camera.Matrix(45.0f, 0.1f, 100.0f, shaderProgram, "camMatrix");

    // Bind the VAO so OpenGL knows to use it | Draw Terrain

    //*############## OpenGL - Draw Calls ################

    terrainGeom.Draw(modelMatrixLocation,terrainModelMatrix);
    vehicleBodyGeom.Draw(modelMatrixLocation, vehicleModelMatrix);

    //! idk why i'm not binding textures and it's still workign...?
    //glBindTexture(GL_TEXTURE_2D, carTexID);

  //Render Same Wheel, but at respective Model Matrix per Wheel 
    for ( glm::mat4 wheelMatrix : wheelMatrices)
    { vehicleWheelGeom.Draw(modelMatrixLocation, wheelMatrix); }

    // Swap the back buffer with the front buffer
    SDL_GL_SwapWindow(Window);

    glm::vec3 cameraPosition = camera.Position;
  }

  glDeleteTextures(1, &carTexID);
  //brickTex.Delete();
  shaderProgram.Delete();

  // TODO: Delete the Physics World Singleton here
}

void XJZoomEngine::Init()
{
  // SDL_ShowSimpleMessageBox(0, "hello", "cppkart", nullptr);
}
